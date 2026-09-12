# ZenOS-RTOS Porting Guide

**Version:** 2.0.0  
**Date:** September 2026

This guide explains how to port ZenOS to a new STM32 family or configure it for a specific board.

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Configuration](#configuration)
3. [Platform Detection](#platform-detection)
4. [Vector Table Setup](#vector-table-setup)
5. [Clock Configuration](#clock-configuration)
6. [FPU Configuration](#fpu-configuration)
7. [MPU Configuration](#mpu-configuration)
8. [IWDG (Watchdog)](#iwdg-watchdog)
9. [CRC Peripheral](#crc-peripheral)
10. [RAM Constraints](#ram-constraints)
11. [Adding New STM32 Families](#adding-new-families)
12. [Cortex-M0/M0+ Specific Notes](#cortex-m0m0-notes)
13. [Cortex-M33 Specific Notes](#cortex-m33-specific-notes)
14. [Common Porting Mistakes](#common-porting-mistakes)

---

## 1. Quick Start

### For existing STM32 families (F1, F4, H7, etc.)

1. Define the STM32 family macro in your project's preprocessor defines:
   ```
   -DSTM32F4xx
   ```

2. Include ZenOS headers in your main application:
   ```cpp
   #include "ZenOS.hpp"
   ```

3. In your CubeMX project, make sure SysTick is configured:
   - `SysTick_Handler` should call `os_tick()`
   - OR rename `SysTick_Handler` to call `os_tick()` via the ZenOS vector table redirect

4. Call `os_init()`, create tasks, then `os_start()`:
   ```cpp
   os_init();
   os_task_create(my_task, 5, 100);  // priority 5, 100ms period
   os_start();  // Never returns
   ```

### For a completely new target

1. Add the family macro to `ZenOS_Port.hpp`
2. Set all hardware-specific macros
3. Verify the build with `arm-none-eabi-g++`
4. Flash and verify UART output

---

## 2. Configuration

All user configuration is in `ZenOS_Config.hpp`. The most important settings:

### Timing

```cpp
// Tick period: 100µs (10kHz) to 1000µs (1kHz)
#define OS_KERNEL_TICK_PERIOD_US  100UL
```

**Impact:** Lower values give better timing resolution but increase tick interrupt overhead.

### Stack Size

```cpp
// Per-task stack size in bytes (must be ≥ 64, multiple of 4)
#define OS_KERNEL_STACK_SIZE  512
```

**How to determine the right size:**
1. Build with `OS_KERNEL_STACK_SIZE = 4096` (generous)
2. Run your application
3. Check `os_get_stack_watermark_percent(task_id)` for peak usage
4. Set `OS_KERNEL_STACK_SIZE = peak × 1.5` (with safety margin)
5. Minimum: 128 bytes for simple tasks, 256 for HAL-using tasks

### Priority Levels

```cpp
// Number of priority slots (2–256, priority 0 reserved)
#define OS_KERNEL_MAX_PRIORITIES  32
```

**RAM impact:** Each priority level uses 4 bytes (queue head pointer). Beyond 32, additional bitmap words are needed (4 bytes per 32 priorities).

### Safety Features

Toggle features based on your safety requirements:

```cpp
#define OS_MONITOR_DEADLINE       1   // Deadline monitoring
#define OS_MONITOR_TCB_INTEGRITY  1   // TCB magic check
#define OS_MONITOR_ERROR_LOG      1   // Error ring buffer
#define OS_SAFETY_SOFT_WATCHDOG   1   // Software watchdog
#define OS_SAFETY_HW_WATCHDOG     1   // Hardware IWDG feed
#define OS_SAFETY_MPU             1   // MPU protection
#define OS_SAFETY_CRC_CHECK       1   // ROM CRC integrity
#define OS_SAFETY_RAM_TEST        1   // Background RAM test
```

---

## 3. Platform Detection

ZenOS detects the target platform via compiler-defined macros:

| Macro | Defined by | Example |
|-------|-----------|---------|
| `STM32F1xx` | `-DSTM32F1xx` | Cortex-M3 target |
| `STM32F4xx` | `-DSTM32F4xx` | Cortex-M4F target |
| `__ARM_ARCH_6M__` | `-march=armv6-m` | Cortex-M0/M0+ |
| `__ARM_ARCH_7M__` | `-march=armv7-m` | Cortex-M3 |
| `__ARM_ARCH_7EM__` | `-march=armv7e-m` | Cortex-M4/M7 |
| `__ARM_ARCH_8M_MAIN__` | `-march=armv8-m.main` | Cortex-M33 |
| `__FPU_PRESENT` | Device header | FPU available |
| `__MPU_PRESENT` | Device header | MPU available |

### Verify Detection

Add to your build flags and check the banner output:
```cpp
uart_send_str("Family: "); uart_send_str(OS_FAMILY_NAME);
uart_send_str("\nArch: "); uart_send_str(OS_ARCH_NAME);
```

---

## 4. Vector Table Setup

ZenOS copies the vector table to RAM at `os_start()` and installs its own handlers:

- `PendSV_Handler` → `OS_PendSV_Handler`
- `HardFault_Handler` → `OS_Fault_Handler`
- `MemManage_Handler` → `OS_Fault_Handler`
- `BusFault_Handler` → `OS_Fault_Handler`
- `UsageFault_Handler` → `OS_Fault_Handler`

### Requirements

1. The startup file must define `SystemCoreClock`
2. SysTick_Handler must call `os_tick()`
3. The vector table must be in flash (loaded by startup)
4. VTOR must be present on Cortex-M3+ (it is on all supported families)

### VTOR-less Targets (Cortex-M0/M0+)

The Cortex-M0 does not have VTOR. The startup vector table at `0x00000000` is used directly. ZenOS handles this automatically — the vector table copy is skipped when `OS_HAS_VTOR == 0`.

---

## 5. Clock Configuration

ZenOS uses `SystemCoreClock` (provided by CMSIS) for timing calculations. Make sure:

1. `SystemCoreClock` is updated after any clock change
2. After clock change, call `os_time_reset()` to resync the µs timing domain

```cpp
SystemClock_Config();        // Your clock config
SystemCoreClockUpdate();     // CMSIS function to recalculate
os_time_reset();             // Resync ZenOS timing
```

---

## 6. FPU Configuration

### When FPU is present (Cortex-M4F, M7, M33)

1. Enable FPU in the project: `-mfloat-abi=hard -mfpu=fpv4-sp-d16`
2. ZenOS automatically detects FPU via `__FPU_PRESENT` / `OS_HAS_FPU_HW`
3. The PendSV handler saves/restores S16-S31 when FPU was used
4. Stack usage increases by ~140 bytes for FPU-using tasks

### When FPU is not present (Cortex-M0, M3)

1. Compile with `-mfloat-abi=soft` (or omit FPU flags)
2. ZenOS sets `OS_HAS_FPU_HW = 0`
3. PendSV does not touch FPU registers
4. No additional stack overhead

### Lazy Stacking

Cortex-M4F/M7 uses lazy FPU stacking:
- Hardware only stacks S0-S15 if FPU was actually used
- ZenOS's PendSV checks `EXC_RETURN` bit 4 to detect this
- Tasks that never use FPU have zero FPU overhead

---

## 7. MPU Configuration

### PMSAv7 (Cortex-M3/M4/M7)

ZenOS programs 3 static regions + 1 per-task region:

| Region | Purpose | Fixed/Programmable |
|--------|---------|-------------------|
| 0 | Flash (code) | Fixed at os_start() |
| 1 | SRAM (data) | Fixed at os_start() |
| 2 | Peripherals | Fixed at os_start() |
| 3 | Task stack | Per-task, in PendSV |

Tasks run unprivileged (`CONTROL.nPRIV=1`). The kernel and ISRs run privileged with `PRIVDEFENA=1` (bypass MPU).

### PMSAv8 (Cortex-M23/M33)

Uses `RLAR` instead of `RASR`. ZenOS automatically detects and uses the correct register layout via `OS_MPU_PMSAv7` / `OS_MPU_PMSAv8` macros.

### Disabling MPU

For targets without MPU hardware:

```cpp
#define OS_SAFETY_MPU  0
```

All MPU code is conditionally compiled. The kernel works without MPU — protection is optional.

---

## 8. IWDG (Watchdog)

### CubeMX Configuration

1. Enable IWDG in CubeMX
2. Set prescaler and reload for desired timeout
3. CubeMX generates `MX_IWDG_Init()`

### OS-Level Integration

ZenOS feeds the watchdog from the idle task when `OS_SAFETY_HW_WATCHDOG = 1`:

- `os_hw_watchdog_feed()` — writes 0xAAAA to IWDG->KR
- `os_hw_watchdog_check()` — conditional feed (skips if errors detected)

### Timeout Calculation

```
IWDG timeout = (4 × 2^prescaler × reload) / LSI_frequency
```

Example: Prescaler=64, Reload=4095, LSI=40kHz
→ timeout = 4 × 64 × 4095 / 40000 ≈ 26.2 seconds

**Rule:** The OS task feeding the watchdog must run within 50% of the IWDG timeout.

---

## 9. CRC Peripheral

### CubeMX Configuration

1. Enable CRC in CubeMX
2. Select the polynomial (default: CRC-32)

### OS-Level Integration

ZenOS uses CRC for ROM integrity verification:
- At boot: computes CRC over entire flash → `crc_expected`
- Periodically (from idle): re-computes and compares

If CRC changes → `os_report_error(HARDFAULT)` — possible ROM corruption.

### Per-Family CRC Addresses

| Family | CRC Base Address | Bus |
|--------|-----------------|-----|
| STM32F1/F2/F3/F4/F7/G4/L0/L1/L4/WB | 0x40023000 | APB1 |
| STM32H7 | 0x58024C00 | AHB4 |

ZenOS automatically uses the correct address via `OS_CRC_BASE_ADDR` in `ZenOS_Port.hpp`.

---

## 10. RAM Constraints

### Minimum RAM by Family

| Family | RAM | Max Tasks (default) |
|--------|-----|---------------------|
| STM32F0 | 8KB | ~10 tasks |
| STM32G0 | 20KB | ~30 tasks |
| STM32L0 | 20KB | ~30 tasks |
| STM32F1 | 20KB | ~30 tasks |
| STM32F4 | 128KB | ~200 tasks |
| STM32H7 | 1MB | ~1500 tasks |

### Reducing RAM Usage

```cpp
#define OS_KERNEL_STACK_SIZE     96     // Reduce per-task stack
#define OS_KERNEL_MAX_PRIORITIES 8      // Reduce priority levels
#define OS_MONITOR_ERROR_LOG_SIZE 8     // Smaller error log
#define OS_MONITOR_ENABLED       0      // Disable all monitoring
#define OS_SAFETY_MPU            0      // No MPU (saves TCB extensions)
#define OS_SAFETY_RAM_TEST       0      // No RAM test
#define OS_SAFETY_CRC_CHECK      0      // No CRC check
```

With minimal config, kernel RAM is ~500 bytes + per-task overhead.

---

## 11. Adding New STM32 Families

To add a new STM32 family (e.g., STM32ZX):

### Step 1: Add Family Detection

In `ZenOS_Port.hpp`, add a new `#elif` block:

```cpp
#elif defined(STM32ZXxx)
    #define OS_FAMILY_NAME      "STM32ZX"
    #define OS_VECTOR_COUNT     XX      // From reference manual
    #define OS_SMP_CORES        1
    #define OS_HAS_FPU_HW       1       // 0 if no FPU
    #define OS_HAS_MPU_HW       1       // 0 if no MPU
    #define OS_FLASH_SIZE       0xXXXXXXUL
    #define OS_RAM_SIZE         0xXXXXXXUL
    #define OS_NVIC_PRIO_BITS   4       // From device header
    #define OS_CRC_BASE_ADDR    0xXXXXXXXUL
    #define OS_IWDG_BASE_ADDR   0xXXXXXXXUL
    #define OS_RCC_CSR_ADDR     0xXXXXXXXUL
```

### Step 2: Verify Values

1. Check the reference manual for:
   - Vector count (number of IRQs + 16)
   - Peripheral base addresses
   - NVIC priority bits

2. Check CMSIS device header for:
   - `__FPU_PRESENT`
   - `__MPU_PRESENT`
   - `__NVIC_PRIO_BITS`

### Step 3: Test

```bash
arm-none-eabi-g++ -mcpu=cortex-mX -mthumb -DSTM32ZXxx \
  -I ZenOS-RTOS/ZenOS \
  -c ZenOS-RTOS/ZenOS/ZenOS.cpp \
  -o /dev/null
```

---

## 12. Cortex-M0/M0+ Specific Notes

Cortex-M0 and M0+ use ARMv6-M, which has significant differences from ARMv7-M:

### Instruction Set
- **Thumb-1 only** (no Thumb-2 instructions)
- No `IT` blocks
- No `SMLAL`/`SMULL`/`UMLAL`/`UMULL`
- No `STRD`/`LDRD`

### Registers
- **No BASEPRI** — cannot mask interrupts by priority
- **No `MRS`/`MSR` for most system registers** — only `PRIMASK`, `CONTROL`, `MSP`, `PSP`, `APSR`
- **No `UXAB`/`CLZ`** — `__builtin_clz()` may not be available (falls back to loop)

### Context Switch
- The PendSV handler uses the same stack frame format (hardware pushes R0-R3, R12, LR, PC, xPSR)
- Software saves R4-R11 (8 words)
- No FPU registers to save

### Priority
- Only 2 bits of priority (4 priority levels)
- `OS_NVIC_PRIO_BITS = 2`
- No sub-priorities

### MPU
- Cortex-M0 has no MPU
- Cortex-M0+ has optional MPU (check `__MPU_PRESENT`)
- If MPU is present on M0+, it uses a simplified region format

### Recommendations for M0/M0+
1. Set `OS_SAFETY_MPU = 0` if no MPU hardware
2. Keep task count low (limited RAM)
3. Avoid deep call stacks (small stacks)
4. Use `os_delay_us()` with busy-wait (no DWT cycle counter)

---

## 13. Cortex-M33 Specific Notes

Cortex-M33 (ARMv8-M Mainline) has enhanced features:

### TrustZone
- Two security zones (Secure/Non-Secure)
- ZenOS runs in Secure zone by default
- MPU supports 16 regions with PMSAv8 register layout

### FPU
- Single or double-precision FPU (check device)
- `OS_HAS_FPU_HW = 1`
- PendSV FPU save/restore works automatically

### MPU (PMSAv8)
- Uses `RLAR` (Region Limit Address Register) instead of `RASR`
- Supports `AttrIndx` for memory type attributes
- 16 regions maximum

### Recommendations for M33
1. Enable `OS_MPU_PMSAv8` (auto-detected)
2. Configure MAIR0/MAIR1 for memory attributes
3. Use double-precision FPU flags if applicable: `-mfpu=fpv5-d16`

---

## 14. Common Porting Mistakes

### ❌ Forgetting to define the STM32 family macro

```bash
# WRONG: no family macro → falls back to "Generic Cortex-M"
arm-none-eabi-g++ -mcpu=cortex-m4 -mthumb ...

# CORRECT:
arm-none-eabi-g++ -mcpu=cortex-m4 -mthumb -DSTM32F4xx ...
```

### ❌ Wrong FPU flags for the target

```bash
# WRONG: FPU flags on a Cortex-M3 (no FPU)
arm-none-eabi-g++ -mcpu=cortex-m3 -mthumb -mfpu=fpv4-sp-d16 ...

# CORRECT:
arm-none-eabi-g++ -mcpu=cortex-m3 -mthumb ...
```

### ❌ Not updating SystemCoreClock after clock change

```cpp
// WRONG: timing will be wrong
SystemClock_Config();
// SystemCoreClock not updated!

// CORRECT:
SystemClock_Config();
SystemCoreClockUpdate();
```

### ❌ Modifying main.cpp

ZenOS test suite is in main.cpp. **Never modify main.cpp** — the kernel must work as-is.

### ❌ Changing SysTick_Handler

ZenOS redirects SysTick through its own vector table. Do not modify `SysTick_Handler()` in `stm32f1xx_it.c` or equivalent — ZenOS handles it.

### ❌ Using 256-entry static arrays

The old approach allocated `TCB* os_pq_head[256]` = 1KB static RAM. ZenOS uses a bitmap + linked list that scales from 32 to 256 priorities with minimal RAM.

---

## Appendix: Build Command Reference

```bash
# Cortex-M0 (STM32F0, soft-float, no MPU)
arm-none-eabi-g++ -mcpu=cortex-m0 -mthumb -march=armv6-m \
  -mfloat-abi=soft -DSTM32F0xx -DOS_SAFETY_MPU=0 -DOS_HAS_FPU_HW=0 ...

# Cortex-M3 (STM32F1, soft-float)
arm-none-eabi-g++ -mcpu=cortex-m3 -mthumb -march=armv7-m \
  -mfloat-abi=soft -DSTM32F1xx ...

# Cortex-M4F (STM32F4, hard-float)
arm-none-eabi-g++ -mcpu=cortex-m4 -mthumb -march=armv7e-m \
  -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32F4xx -DOS_HAS_FPU_HW=1 ...

# Cortex-M7F (STM32H7, hard-float, double precision)
arm-none-eabi-g++ -mcpu=cortex-m7 -mthumb -march=armv7e-m \
  -mfloat-abi=hard -mfpu=fpv5-d16 -DSTM32H7xx -DOS_HAS_FPU_HW=1 ...

# Cortex-M33 (STM32L5, hard-float)
arm-none-eabi-g++ -mcpu=cortex-m33 -mthumb -march=armv8-m.main \
  -mfloat-abi=hard -mfpu=fpv5-sp-d16 -DSTM32L5xx -DOS_HAS_FPU_HW=1 ...
```

---

*End of Porting Guide*
