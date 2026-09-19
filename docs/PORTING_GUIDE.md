# ZenOS Porting Guide — Adding a New STM32 Family

This guide explains how to add support for a new STM32 chip family to ZenOS RTOS.

## Prerequisites

- The target chip's reference manual (RM) for register addresses
- ARM GCC toolchain (`arm-none-eabi-gcc`)
- A linker script for your target chip

## Step 1: Add Family Detection in `ZenOS_Port.hpp`

Add a new `#elif defined(STM32XXXX)` block in the STM32 Family Detection section:

```cpp
#elif defined(STM32XXXX)
    #define OS_FAMILY_NAME      "STM32XXXX"
    #define OS_VECTOR_COUNT     NNN      /* From RM, number of IRQs + 16 */
    #define OS_SMP_CORES        1        /* 1 for single-core */
    /* NOTE: do NOT define OS_HAS_FPU_HW / OS_HAS_MPU_HW here.
       Capability flags are derived automatically from the CMSIS device
       header (__FPU_PRESENT / __MPU_PRESENT) and the compiler's
       __ARM_ARCH_* macros — a family name alone cannot prove that the
       silicon vendor instantiated an optional component. */
    #define OS_FLASH_SIZE       0xNNNNN  /* Typical flash size in bytes */
    #define OS_RAM_SIZE         0xNNNN   /* Typical SRAM size in bytes */
    #define OS_NVIC_PRIO_BITS   N        /* 2, 3, or 4 */
    #define OS_CRC_BASE_ADDR    0xNNNNNNNN  /* From RM */
    #define OS_IWDG_BASE_ADDR   0xNNNNNNNN  /* From RM */
    #define OS_RCC_CSR_ADDR     0xNNNNNNNN  /* From RM */
```

## Step 2: Determine Key Register Addresses

From the target chip's Reference Manual, find:

| Register | Typical Addresses | Notes |
|----------|------------------|-------|
| **RCC CSR** (reset flags) | `0x40021024` (F1) | `0x580244D0` for H7 |
| **CRC DR** | `0x40023000` | `0x58024C00` for H7 (AHB4 bus) |
| **IWDG KR** | `0x40003000` | `0x58004800` for H7 (APB4 bus) |
| **USART1** | `0x40013800` (APB2) | Varies by family |

## Step 3: Determine Architecture

Check the Cortex-M core:

| Core | `__ARM_ARCH_*__` | Has DWT? | Has MPU? | Has FPU? | Has VTOR? |
|------|-----------------|----------|----------|----------|-----------|
| M0 | `__ARM_ARCH_6M__` | No | No | No | No |
| M0+ | `__ARM_ARCH_6M__` | No | Optional | No | No |
| M3 | `__ARM_ARCH_7M__` | Yes | Yes | No | Yes |
| M4 | `__ARM_ARCH_7EM__` | Yes | Yes | Optional | Yes |
| M7 | `__ARM_ARCH_7EM__` | Yes | Yes | Yes | Yes |
| M23 | `__ARM_ARCH_8M_BASE__` | Yes | Yes (PMSAv8) | Optional | Yes |
| M33 | `__ARM_ARCH_8M_MAIN__` | Yes | Yes (PMSAv8) | Yes | Yes |

## Step 4: Create Startup File

Create `startup_stm32xxxx.s` with:
- Vector table (match `OS_VECTOR_COUNT`)
- Weak aliases for all IRQ handlers
- `Reset_Handler` with `.data` init, `.bss` zero, and `main()` call

Use the existing `startup_stm32f103c8tx.S` as a template.

## Step 5: Create Linker Script

Create `STM32xxxx_FLASH.ld` with:
- Flash origin and length (`OS_FLASH_SIZE`)
- RAM origin and length (`OS_RAM_SIZE`)
- `.text`, `.data`, `.bss`, `_estack` sections

## Step 6: Handle Exception Differences

- **ARMv6-M (M0/M0+)**: No VTOR, no BASEPRI, Thumb-1 only
  - Use weak symbol override in startup for PendSV/Fault handlers
  - `os_yield()` uses PRIMASK (no BASEPRI)
- **ARMv7-M+ (M3/M4/M7)**: Full exception model
  - Use VTOR relocation in `os_start()` to install handlers
  - PendSV and SysTick priority set via SHPR3

## Step 7: Verify

1. **Build**: Compile with `-mcpu=cortex-mN` and verify zero warnings
2. **LED test**: Create a minimal `main()` that toggles an LED via raw registers
3. **SysTick test**: Verify `os_get_tick()` increments correctly
4. **Task test**: Create two tasks with different priorities, verify preemption
5. **Console test**: Initialise a UART with the HAL (or raw registers) and send
   a short string from a task. ZenOS has no built-in debug UART — the
   application owns its console.

## Optional: Enable Advanced Features

| Feature | Config Macro | Notes |
|---------|-------------|-------|
| MPU protection | `OS_SAFETY_MPU_EN=1` | Requires PMSAv7/PMSAv8; rejected at compile time when `__MPU_PRESENT=0` |
| Hardware watchdog | `OS_SAFETY_HW_WATCHDOG_EN=1` | Configure IWDG timeout in CubeMX/init |
| CRC integrity | `OS_SAFETY_CRC_EN=1` | Must match flash contents |
| RAM test | `OS_SAFETY_RAM_TEST_EN=1` | Call `os_ram_test_step()` from idle |
| Tickless idle | `OS_KERNEL_TICKLESS_IDLE_EN=1` | Power savings via WFI; see the idle-stack note below |

## Idle stack sizing

`OS_IDLE_STACK_WORDS` (default 128 words = 512 bytes) sizes the idle task stack.
The idle loop is not a bare `wfi` — it runs the hardware-watchdog check, the
CRC step and the tickless-idle path, and any interrupt taken while idle pushes
a full exception frame onto the same stack. Measure the idle peak with
`os_get_stack_watermark(255)` on the target before reducing this value.
