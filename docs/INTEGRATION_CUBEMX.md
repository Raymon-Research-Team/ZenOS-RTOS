# ZenOS Integration with CubeMX Projects

**Version:** 1.1.0  
**Date:** September 2026

---

## Overview

ZenOS can coexist with STM32 HAL/LL/CMSIS in CubeMX-generated projects.
This document defines the coexistence contract — what to configure, what
to avoid, and how to verify correct integration.

---

## 1. Tick Timer Allocation

**Rule:** SysTick belongs to ZenOS. HAL timebase uses a hardware timer.

### Recommended Setup (CubeMX)

1. **SYS → Timebase Source:** Select `TIM1` (or any free timer) instead of `SysTick`
2. **NVIC → SysTick interrupt:** Leave enabled (ZenOS handles it)
3. **NVIC → TIM1 update interrupt:** Enabled (HAL timebase)

### Why This Matters

- `SysTick_Handler()` calls `os_tick()` directly — HAL must not also use it
- HAL's `HAL_IncTick()` runs in `TIM1_UP_IRQHandler()` via `HAL_TIM_PeriodElapsedCallback()`
- This separation is already demonstrated in the sample `stm32f1xx_it.c`

### What Happens If Both Use SysTick

If HAL timebase remains on SysTick, `HAL_IncTick()` and `os_tick()` both
fire on the same interrupt. This causes:
- `uwTick` and `tick_count` advancing at double rate
- HAL timeouts halving unexpectedly
- Deadline monitoring producing false positives

**This configuration is explicitly unsupported.**

---

## 2. Vector Table Relocation

**Rule:** `os_start()` must be called AFTER all peripherals that install
ISRs have been initialized.

### How It Works

```c
os_start() copies the current VTOR table to RAM, replaces PendSV and
fault handler vectors, then redirects VTOR to the RAM copy. Any ISR
installed by HAL before os_start() is preserved. Any ISR installed
after os_start() is NOT in the RAM table.
```

### Safe Sequence

```c
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();       // Installs EXTI ISRs via NVIC
    MX_USART1_UART_Init();
    MX_SPI1_Init();
    // ... all HAL init done ...

    os_init();             // ZenOS kernel init
    os_task_create(...);   // Create tasks
    os_start();            // Takes over — never returns
}
```

### Late ISR Installation

If you must install an ISR after `os_start()`, call:
```c
os_kernel_relocate_vectors();  // Re-copies VTOR to RAM (TODO: add API)
```
Or install ISRs before `os_start()`.

---

## 3. Interrupt Priorities

**Rule:** PendSV and SysTick must be at the lowest priority.

### ZenOS Default Priorities

| Handler | Priority | Register |
|---------|----------|----------|
| PendSV  | 0xFE     | SHPR3[23:16] |
| SysTick | 0xFF     | SHPR3[31:24] |

### HAL ISRs

HAL ISRs (EXTI, UART, SPI, ADC, etc.) run at priorities 0–5 by default.
Since PendSV is at 0xFE and SysTick at 0xFF, HAL ISRs always preempt
the scheduler — this is correct behavior.

### What NOT To Do

- Do not set HAL ISR priority to 0xFE or 0xFF — this would preempt the
  scheduler and corrupt context switch state
- Do not call `HAL_NVIC_SetPriority(PendSV_IRQn, ...)` after `os_start()`

---

## 4. Watchdog Ownership

**Rule:** Either HAL feeds IWDG or ZenOS does — never both.

### Option A: ZenOS Owns Watchdog

```c
// In ZenOS_Config.hpp:
#define OS_SAFETY_HW_WATCHDOG  1

// In CubeMX: disable IWDG auto-feed in HAL
// (MX_IWDG_Init() still configures prescaler/reload)
```

### Option B: HAL Owns Watchdog

```c
// In ZenOS_Config.hpp:
#define OS_SAFETY_HW_WATCHDOG  0

// Application code: feed in main loop or dedicated task
HAL_IWDG_Refresh(&hiwdg);
```

### Verification

Check `os_hw_watchdog_feed()` is called exactly once per IWDG timeout
period. If both HAL and ZenOS feed, the effective timeout becomes
unpredictable.

---

## 5. UART Ownership

**Rule:** ZenOS does not own any UART. The kernel has no debug UART and no
printf-style logging path; the earlier `OS_DEBUG_UART` debug subsystem is not
part of the current source tree. Every UART belongs to the application.

- Configure and use USARTs through CubeMX/HAL exactly as you would without
  ZenOS. No pins are reserved by the kernel.
- Use `os_log_error()` / `os_get_error_log_entry()` for structured kernel
  error reporting (RAM ring buffer, see `OS_MONITOR_ERROR_LOG`); the
  application is responsible for forwarding those entries to a console if
  it wants textual output.
- The sample application (`Core/Src/main.cpp`) implements its own small
  `uart_send_str()`/`uart_send_uint()` helpers over `HAL_UART_Transmit`;
  copy that pattern rather than expecting a kernel API.

---

## 6. HAL-Free Alternative

For projects that want zero HAL dependency, use the register-level
`main.cpp` and `stm32f1xx_it.c` provided with ZenOS.

### What This Provides

- SystemClock_Config via direct RCC register writes
- GPIO init via direct register writes
- SysTick_Handler → os_tick() (no HAL_IncTick)
- EXTI handler with register-level pending-bit clear
- TIM1 handler with register-level flag clear

### What You Lose

- CubeMX code generation for peripherals
- HAL driver API (HAL_UART_Transmit, HAL_SPI_*, etc.)
- CubeMX's graphical pin configuration

### When to Use Each

| Use Case | Approach |
|----------|----------|
| New project, full control | Register-level (no HAL) |
| Existing CubeMX project | HAL coexistence (this guide) |
| Mixed: ZenOS + HAL peripherals | HAL coexistence + USER CODE sections |

---

## 7. Verification Checklist

After integration, verify:

- [ ] `SysTick_Handler()` calls only `os_tick()` (no HAL_IncTick)
- [ ] `HAL_IncTick()` is called from TIMx_IRQHandler
- [ ] `os_start()` is called after all `MX_*_Init()` functions
- [ ] PendSV priority = 0xFE, SysTick priority = 0xFF
- [ ] No HAL ISR has priority ≥ 0xFE
- [ ] IWDG feed is owned by exactly one subsystem
- [ ] Application UARTs are initialised after `os_start()` is not required
      (init them before, as in the sample)
- [ ] Build succeeds with `-Wall -Wextra` (kernel sources are warning-clean)

---

## 8. Sample CubeMX Project Structure

```
MyProject/
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   └── stm32f1xx_hal_conf.h
│   ├── Src/
│   │   ├── main.cpp              # Modified: USER CODE calls for ZenOS
│   │   ├── stm32f1xx_it.c        # Modified: os_tick() in SysTick_Handler
│   │   ├── stm32f1xx_hal_msp.c   # CubeMX generated (untouched)
│   │   └── system_stm32f1xx.c    # CubeMX generated (untouched)
│   └── ZenOS-RTOS/               # ZenOS kernel (untouched)
│       └── ZenOS/
│           ├── ZenOS.hpp
│           ├── ZenOS.cpp
│           └── ...
├── Drivers/                       # STM32 HAL/CMSIS (CubeMX generated)
└── STM32F103C8TX_FLASH.ld         # Linker script
```

---

*End of Integration Guide*
