#pragma once
/**
 * @file    ZenOS_port.hpp
 * @brief   Platform-specific definitions for ARM Cortex-M
 *
 * CMSIS device capability defines are authoritative whenever available.
 */

#define OS_NAKED  __attribute__((naked))
#define OS_USED   __attribute__((used))
#define OS_ALIGNED(n) __attribute__((aligned(n)))

#if defined(STM32F1xx)
    #define OS_FAMILY_NAME "STM32F1"
    #define OS_VECTOR_COUNT 68
    #define OS_SMP_CORES 1
#elif defined(STM32F2xx)
    #define OS_FAMILY_NAME "STM32F2"
    #define OS_VECTOR_COUNT 81
    #define OS_SMP_CORES 1
#elif defined(STM32F4xx)
    #define OS_FAMILY_NAME "STM32F4"
    #define OS_VECTOR_COUNT 86
    #define OS_SMP_CORES 1
#elif defined(STM32F7xx)
    #define OS_FAMILY_NAME "STM32F7"
    #define OS_VECTOR_COUNT 91
    #define OS_SMP_CORES 1
#elif defined(STM32H7xx)
    #define OS_FAMILY_NAME "STM32H7"
    #define OS_VECTOR_COUNT 150
    #define OS_SMP_CORES 1
#elif defined(STM32G4xx)
    #define OS_FAMILY_NAME "STM32G4"
    #define OS_VECTOR_COUNT 100
    #define OS_SMP_CORES 1
#elif defined(STM32L0xx)
    #define OS_FAMILY_NAME "STM32L0"
    #define OS_VECTOR_COUNT 32
    #define OS_SMP_CORES 1
#elif defined(STM32L4xx)
    #define OS_FAMILY_NAME "STM32L4"
    #define OS_VECTOR_COUNT 82
    #define OS_SMP_CORES 1
#elif defined(STM32WBxx)
    #define OS_FAMILY_NAME "STM32WB"
    #define OS_VECTOR_COUNT 66
    #define OS_SMP_CORES 1
#elif defined(STM32G0xx)
    #define OS_FAMILY_NAME "STM32G0"
    #define OS_VECTOR_COUNT 32
    #define OS_SMP_CORES 1
#else
    #define OS_FAMILY_NAME "Generic Cortex-M"
    #define OS_VECTOR_COUNT 68
    #define OS_SMP_CORES 1
#endif
#define OS_SMP_MAX_CORES 2

#define OS_STACK_CANARY 0xDEADBEEFUL
#define OS_STACK_CANARY_COUNT 8
#define OS_TCB_MAGIC 0x54434200UL

#define OS_FLASH_START 0x08000000UL
#if defined(STM32F1xx)
 #define OS_FLASH_SIZE 0x10000UL
#elif defined(STM32F2xx)
 #define OS_FLASH_SIZE 0x100000UL
#elif defined(STM32F4xx)
 #define OS_FLASH_SIZE 0x100000UL
#elif defined(STM32F7xx)
 #define OS_FLASH_SIZE 0x200000UL
#elif defined(STM32H7xx)
 #define OS_FLASH_SIZE 0x200000UL
#elif defined(STM32G4xx)
 #define OS_FLASH_SIZE 0x80000UL
#elif defined(STM32L0xx)
 #define OS_FLASH_SIZE 0x20000UL
#elif defined(STM32L4xx)
 #define OS_FLASH_SIZE 0x100000UL
#elif defined(STM32WBxx)
 #define OS_FLASH_SIZE 0x100000UL
#elif defined(STM32G0xx)
 #define OS_FLASH_SIZE 0x20000UL
#else
 #define OS_FLASH_SIZE 0x20000UL
#endif

#if defined(__ARM_ARCH_7M__)
 #define OS_MPU_MAX_REGIONS 8
#elif defined(__ARM_ARCH_7EM__)
 #define OS_MPU_MAX_REGIONS 8
#elif defined(__ARM_ARCH_8M_MAIN__)
 #define OS_MPU_MAX_REGIONS 16
#else
 #define OS_MPU_MAX_REGIONS 8
#endif

#define OS_RAM_START 0x20000000UL
#if defined(STM32F1xx)
 #define OS_RAM_SIZE 0x5000UL
#elif defined(STM32F2xx)
 #define OS_RAM_SIZE 0x20000UL
#elif defined(STM32F4xx)
 #define OS_RAM_SIZE 0x20000UL
#elif defined(STM32F7xx)
 #define OS_RAM_SIZE 0x40000UL
#elif defined(STM32H7xx)
 #define OS_RAM_SIZE 0x40000UL
#elif defined(STM32G4xx)
 #define OS_RAM_SIZE 0x18000UL
#elif defined(STM32L0xx)
 #define OS_RAM_SIZE 0x5000UL
#elif defined(STM32L4xx)
 #define OS_RAM_SIZE 0x20000UL
#elif defined(STM32WBxx)
 #define OS_RAM_SIZE 0x40000UL
#elif defined(STM32G0xx)
 #define OS_RAM_SIZE 0x5000UL
#else
 #define OS_RAM_SIZE 0x5000UL
#endif

#define OS_RAM_TEST_SKIP 0x4000UL
#define OS_RAM_TEST_START (OS_RAM_START + OS_RAM_TEST_SKIP)
#define OS_RAM_TEST_END (OS_RAM_START + (OS_RAM_SIZE / 2))

#define OS_HARDFAULT_VECTOR_INDEX 3
#define OS_MEMMANAGE_VECTOR_INDEX 4
#define OS_BUSFAULT_VECTOR_INDEX 5
#define OS_USAGEFAULT_VECTOR_INDEX 6
#define OS_PENDSV_VECTOR_INDEX 14
#define OS_SYSTICK_VECTOR_INDEX 15

#define OS_PENDSV_PRIORITY ((1UL << __NVIC_PRIO_BITS) - 1UL)
#define OS_SYSTICK_PRIORITY ((1UL << __NVIC_PRIO_BITS) - 1UL)
#define OS_PENDSV_PRIO (*((volatile uint8_t*)0xE000ED22UL))
#define OS_SYSTICK_PRIO (*((volatile uint8_t*)0xE000ED23UL))

#define OS_SCB_BASE 0xE000ED00UL
#define OS_SCB_ICSR (*((volatile uint32_t*)(OS_SCB_BASE + 0x04UL)))
#define OS_SCB_VTOR (*((volatile uint32_t*)(OS_SCB_BASE + 0x08UL)))
#define OS_SCB_SHPR3 (*((volatile uint32_t*)(OS_SCB_BASE + 0x20UL)))
#define OS_ICSR_PENDSVSET_Msk (1UL << 28)

#define OS_SYST_BASE 0xE000E010UL
#define OS_SYST_CSR (*((volatile uint32_t*)(OS_SYST_BASE + 0x00UL)))
#define OS_SYST_RVR (*((volatile uint32_t*)(OS_SYST_BASE + 0x04UL)))
#define OS_SYST_CVR (*((volatile uint32_t*)(OS_SYST_BASE + 0x08UL)))
#define OS_SYST_CSR_ENABLE_Msk (1UL << 0)
#define OS_SYST_CSR_TICKINT_Msk (1UL << 1)
#define OS_SYST_CSR_COUNTFLAG_Msk (1UL << 16)

#define OS_DWT_BASE 0xE0001000UL
#define OS_DWT_CTRL (*((volatile uint32_t*)(OS_DWT_BASE + 0x00UL)))
#define OS_DWT_CYCCNT (*((volatile uint32_t*)(OS_DWT_BASE + 0x04UL)))
#define OS_DWT_CTRL_CYCCNTENA_Msk (1UL << 0)
#if defined(DWT) || defined(DWT_LSR)
 #define OS_HAS_CYCLE_COUNTER 1
#elif defined(STM32F1xx) || defined(STM32F2xx) || defined(STM32F3xx) || defined(STM32F4xx) || defined(STM32F7xx) || defined(STM32H7xx) || defined(STM32G4xx) || defined(STM32L4xx) || defined(STM32L1xx)
 #define OS_HAS_CYCLE_COUNTER 1
#else
 #define OS_HAS_CYCLE_COUNTER 0
#endif
#define OS_COREDEBUG_DEMCR (*(volatile uint32_t*)0xE000EDFCUL)
#define OS_COREDEM_TRCENA (1UL << 24)
#define OS_DWT_CYCCNTENA (1UL << 0)

/* Device-header feature detection. CMSIS explicitly defines these as
   device capabilities, not architectural guarantees. */
#if defined(__MPU_PRESENT)
 #if (__MPU_PRESENT == 0U)
  #undef OS_SAFETY_MPU
  #define OS_SAFETY_MPU 0
 #endif
#else
 #undef OS_SAFETY_MPU
 #define OS_SAFETY_MPU 0
#endif

#if defined(__VTOR_PRESENT)
 #define OS_HAS_VTOR ((__VTOR_PRESENT) != 0U)
#elif defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_8M_BASE__)
 #define OS_HAS_VTOR 0
#else
 #define OS_HAS_VTOR 1
#endif

#if defined(__FPU_PRESENT)
 #define OS_HAS_FPU ((__FPU_PRESENT) != 0U)
#else
 #define OS_HAS_FPU 0
#endif
