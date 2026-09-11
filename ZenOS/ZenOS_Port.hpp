#pragma once
/**
 * @file    ZenOS_Port.hpp
 * @brief   Cortex-M capability and memory-map layer.
 *
 * Device-header capability macros are authoritative.  Board-specific memory
 * sizes and optional safety peripherals may be supplied by the build system;
 * ZenOS never guesses a physical RAM/Flash size when an explicit override is
 * available.
 */

#define OS_NAKED  __attribute__((naked))
#define OS_USED   __attribute__((used))
#define OS_ALIGNED(n) __attribute__((aligned(n)))

/* --------------------------------------------------------------------------
 * Core capability detection
 * -------------------------------------------------------------------------- */
#if defined(__ARM_ARCH_6M__)
# define OS_ARCH_ARMV6M 1
#else
# define OS_ARCH_ARMV6M 0
#endif

#if defined(__ARM_ARCH_7M__)
# define OS_ARCH_ARMV7M 1
#else
# define OS_ARCH_ARMV7M 0
#endif

#if defined(__ARM_ARCH_7EM__)
# define OS_ARCH_ARMV7EM 1
#else
# define OS_ARCH_ARMV7EM 0
#endif

#if defined(__ARM_ARCH_8M_BASE__)
# define OS_ARCH_ARMV8MBL 1
#else
# define OS_ARCH_ARMV8MBL 0
#endif

#if defined(__ARM_ARCH_8M_MAIN__)
# define OS_ARCH_ARMV8MML 1
#else
# define OS_ARCH_ARMV8MML 0
#endif

#if defined(__ARM_ARCH_8_1M_MAIN__)
# define OS_ARCH_ARMV81MML 1
#else
# define OS_ARCH_ARMV81MML 0
#endif

/* Cortex-M0/M0+ do not implement PendSV.  ZenOS intentionally fails at
   compile time on Armv6-M rather than generating an image whose vector table
   and context-switch mechanism cannot work.  Cortex-M23 (Armv8-M Baseline)
   does implement the RTOS exception set and is handled separately by the
   Armv8-M baseline port. */
#if OS_ARCH_ARMV6M
# define OS_HAS_PENDSV 0
#else
# define OS_HAS_PENDSV 1
#endif

#if defined(__MPU_PRESENT)
# define OS_HAS_MPU ((__MPU_PRESENT) != 0U)
#else
# define OS_HAS_MPU 0
#endif

#if defined(__VTOR_PRESENT)
# define OS_HAS_VTOR ((__VTOR_PRESENT) != 0U)
#elif OS_ARCH_ARMV6M || OS_ARCH_ARMV8MBL
# define OS_HAS_VTOR 0
#else
# define OS_HAS_VTOR 1
#endif

#if defined(__FPU_PRESENT)
# define OS_HAS_FPU ((__FPU_PRESENT) != 0U)
#else
# define OS_HAS_FPU 0
#endif

#if defined(__SAUREGION_PRESENT)
# define OS_HAS_SAU ((__SAUREGION_PRESENT) != 0U)
#else
# define OS_HAS_SAU 0
#endif

/* Armv8-M Mainline has a different MPU register model (RBAR/RLAR) from
   Armv7-M (RBAR/RASR).  Do not accidentally program the v7-M register map. */
#if OS_ARCH_ARMV8MML || OS_ARCH_ARMV81MML
# define OS_MPU_PMSA_V8 1
#else
# define OS_MPU_PMSA_V8 0
#endif

/* --------------------------------------------------------------------------
 * Target identification
 * -------------------------------------------------------------------------- */
#if defined(STM32F1xx)
# define OS_FAMILY_NAME "STM32F1"
#elif defined(STM32F2xx)
# define OS_FAMILY_NAME "STM32F2"
#elif defined(STM32F3xx)
# define OS_FAMILY_NAME "STM32F3"
#elif defined(STM32F4xx)
# define OS_FAMILY_NAME "STM32F4"
#elif defined(STM32F7xx)
# define OS_FAMILY_NAME "STM32F7"
#elif defined(STM32G0xx)
# define OS_FAMILY_NAME "STM32G0"
#elif defined(STM32G4xx)
# define OS_FAMILY_NAME "STM32G4"
#elif defined(STM32H5xx)
# define OS_FAMILY_NAME "STM32H5"
#elif defined(STM32H7xx)
# define OS_FAMILY_NAME "STM32H7"
#elif defined(STM32L0xx)
# define OS_FAMILY_NAME "STM32L0"
#elif defined(STM32L1xx)
# define OS_FAMILY_NAME "STM32L1"
#elif defined(STM32L4xx)
# define OS_FAMILY_NAME "STM32L4"
#elif defined(STM32WBxx)
# define OS_FAMILY_NAME "STM32WB"
#elif defined(STM32WLxx)
# define OS_FAMILY_NAME "STM32WL"
#else
# define OS_FAMILY_NAME "Generic Cortex-M"
#endif

#ifndef OS_SMP_CORES
# define OS_SMP_CORES 1
#endif
#define OS_SMP_MAX_CORES 1

/* --------------------------------------------------------------------------
 * Device memory map
 *
 * A project may define OS_FLASH_START/OS_FLASH_SIZE/OS_RAM_START/OS_RAM_SIZE
 * in compiler definitions or before this header.  STM32 CMSIS device headers
 * commonly provide FLASH_SIZE/SRAM_SIZE; use those when available.  The
 * remaining fallback is deliberately conservative and is never used by the
 * supported STM32 targets when the device header exports its sizes.
 * -------------------------------------------------------------------------- */
#ifndef OS_FLASH_START
# define OS_FLASH_START 0x08000000UL
#endif

#ifndef OS_FLASH_SIZE
# if defined(FLASH_SIZE)
#  define OS_FLASH_SIZE ((uint32_t)(FLASH_SIZE))
# elif defined(STM32F1xx)
#  define OS_FLASH_SIZE 0x10000UL
# elif defined(STM32F2xx) || defined(STM32F4xx)
#  define OS_FLASH_SIZE 0x100000UL
# elif defined(STM32F7xx) || defined(STM32H7xx)
#  define OS_FLASH_SIZE 0x200000UL
# elif defined(STM32G4xx)
#  define OS_FLASH_SIZE 0x80000UL
# elif defined(STM32L0xx)
#  define OS_FLASH_SIZE 0x20000UL
# elif defined(STM32L4xx) || defined(STM32WBxx)
#  define OS_FLASH_SIZE 0x100000UL
# else
#  define OS_FLASH_SIZE 0x20000UL
# endif
#endif

#ifndef OS_RAM_START
# define OS_RAM_START 0x20000000UL
#endif

#ifndef OS_RAM_SIZE
# if defined(SRAM_SIZE)
#  define OS_RAM_SIZE ((uint32_t)(SRAM_SIZE))
# elif defined(STM32F1xx)
#  define OS_RAM_SIZE 0x5000UL
# elif defined(STM32F2xx) || defined(STM32F4xx)
#  define OS_RAM_SIZE 0x20000UL
# elif defined(STM32F7xx) || defined(STM32H7xx)
#  define OS_RAM_SIZE 0x40000UL
# elif defined(STM32G4xx)
#  define OS_RAM_SIZE 0x18000UL
# elif defined(STM32L0xx)
#  define OS_RAM_SIZE 0x5000UL
# elif defined(STM32L4xx)
#  define OS_RAM_SIZE 0x20000UL
# elif defined(STM32WBxx)
#  define OS_RAM_SIZE 0x40000UL
# else
#  define OS_RAM_SIZE 0x5000UL
# endif
#endif

/* RAM tests are enabled only when the application supplies a linker-defined
   scratch range.  Never write into an arbitrary percentage of live SRAM. */
#ifndef OS_RAM_TEST_START
# define OS_RAM_TEST_START 0UL
#endif
#ifndef OS_RAM_TEST_END
# define OS_RAM_TEST_END 0UL
#endif
#if (OS_RAM_TEST_START != 0UL) && (OS_RAM_TEST_END > OS_RAM_TEST_START)
# define OS_RAM_TEST_REGION_VALID 1
#else
# define OS_RAM_TEST_REGION_VALID 0
#endif

/* --------------------------------------------------------------------------
 * MPU capability
 * -------------------------------------------------------------------------- */
#if defined(__MPU_REGION_COUNT)
# define OS_MPU_MAX_REGIONS (__MPU_REGION_COUNT)
#elif defined(__ARM_ARCH_8M_MAIN__) || defined(__ARM_ARCH_8_1M_MAIN__)
# define OS_MPU_MAX_REGIONS 16
#else
# define OS_MPU_MAX_REGIONS 8
#endif

/* The default config historically enabled MPU unconditionally.  Override it
   from the actual device capability before the public API is parsed. */
#if !OS_HAS_MPU
# undef OS_SAFETY_MPU
# define OS_SAFETY_MPU 0
#endif

/* --------------------------------------------------------------------------
 * Core exception/vector registers
 * -------------------------------------------------------------------------- */
#define OS_HARDFAULT_VECTOR_INDEX 3
#define OS_MEMMANAGE_VECTOR_INDEX 4
#define OS_BUSFAULT_VECTOR_INDEX 5
#define OS_USAGEFAULT_VECTOR_INDEX 6
#define OS_PENDSV_VECTOR_INDEX 14
#define OS_SYSTICK_VECTOR_INDEX 15

#ifndef OS_VECTOR_COUNT
/* A vector table is device/startup-file specific.  16 core vectors is the
   only architectural count that can be assumed; projects with external IRQs
   should define OS_VECTOR_COUNT to their startup table length. */
# define OS_VECTOR_COUNT 16
#endif

#if defined(__NVIC_PRIO_BITS)
# define OS_PENDSV_PRIORITY ((1UL << __NVIC_PRIO_BITS) - 1UL)
# define OS_SYSTICK_PRIORITY ((1UL << __NVIC_PRIO_BITS) - 1UL)
#else
# define OS_PENDSV_PRIORITY 0xFFUL
# define OS_SYSTICK_PRIORITY 0xFFUL
#endif

#define OS_SCB_BASE 0xE000ED00UL
#define OS_SCB_ICSR (*((volatile uint32_t*)(OS_SCB_BASE + 0x04UL)))
#if OS_HAS_VTOR
# define OS_SCB_VTOR (*((volatile uint32_t*)(OS_SCB_BASE + 0x08UL)))
#endif
#define OS_SCB_SHPR3 (*((volatile uint32_t*)(OS_SCB_BASE + 0x20UL)))
#define OS_ICSR_PENDSVSET_Msk (1UL << 28)
#define OS_SCB_SHPR2 (*((volatile uint32_t*)(OS_SCB_BASE + 0x1CUL)))
#define OS_PENDSV_PRIO (*((volatile uint8_t*)(OS_SCB_BASE + 0x22UL)))
#define OS_SYSTICK_PRIO (*((volatile uint8_t*)(OS_SCB_BASE + 0x23UL)))

#define OS_SYST_BASE 0xE000E010UL
#define OS_SYST_CSR (*((volatile uint32_t*)(OS_SYST_BASE + 0x00UL)))
#define OS_SYST_RVR (*((volatile uint32_t*)(OS_SYST_BASE + 0x04UL)))
#define OS_SYST_CVR (*((volatile uint32_t*)(OS_SYST_BASE + 0x08UL)))
#define OS_SYST_CSR_ENABLE_Msk (1UL << 0)
#define OS_SYST_CSR_TICKINT_Msk (1UL << 1)
#define OS_SYST_CSR_COUNTFLAG_Msk (1UL << 16)

/* --------------------------------------------------------------------------
 * DWT capability
 * -------------------------------------------------------------------------- */
#define OS_DWT_BASE 0xE0001000UL
#define OS_DWT_CTRL (*((volatile uint32_t*)(OS_DWT_BASE + 0x00UL)))
#define OS_DWT_CYCCNT (*((volatile uint32_t*)(OS_DWT_BASE + 0x04UL)))
#define OS_DWT_CTRL_CYCCNTENA_Msk (1UL << 0)
#if defined(DWT) || defined(DWT_LSR) || defined(STM32F1xx) || defined(STM32F2xx) || defined(STM32F3xx) || defined(STM32F4xx) || defined(STM32F7xx) || defined(STM32H7xx) || defined(STM32G4xx) || defined(STM32L1xx) || defined(STM32L4xx)
# define OS_HAS_CYCLE_COUNTER 1
#else
# define OS_HAS_CYCLE_COUNTER 0
#endif
#define OS_COREDEBUG_DEMCR (*(volatile uint32_t*)0xE000EDFCUL)
#define OS_COREDEM_TRCENA (1UL << 24)
#define OS_DWT_CYCCNTENA (1UL << 0)

/* --------------------------------------------------------------------------
 * Platform safety peripherals.  They are opt-in because peripheral addresses
 * and clock/reset flags are vendor/device specific.
 * -------------------------------------------------------------------------- */
#ifndef OS_PLATFORM_WATCHDOG
# if defined(STM32F1xx) || defined(STM32F2xx) || defined(STM32F3xx) || defined(STM32F4xx) || defined(STM32F7xx) || defined(STM32G0xx) || defined(STM32G4xx) || defined(STM32L0xx) || defined(STM32L1xx) || defined(STM32L4xx) || defined(STM32WBxx) || defined(STM32WLxx)
#  define OS_PLATFORM_WATCHDOG 1
# else
#  define OS_PLATFORM_WATCHDOG 0
# endif
#endif

#ifndef OS_PLATFORM_CRC
# define OS_PLATFORM_CRC 0
#endif

/* No valid scratch RAM => no RAM writes. */
#if !OS_RAM_TEST_REGION_VALID
# undef OS_SAFETY_RAM_TEST
# define OS_SAFETY_RAM_TEST 0
#endif

/* No platform CRC implementation => do not pretend that a hard-coded STM32F1
   CRC peripheral is portable. */
#if !OS_PLATFORM_CRC
# undef OS_SAFETY_CRC_CHECK
# define OS_SAFETY_CRC_CHECK 0
#endif

/* Armv6-M has no PendSV; the existing ZenOS public contract deliberately does
   not rewrite the application's SysTick handler.  Fail closed instead of
   producing a silently nonfunctional scheduler port. */
#if !OS_HAS_PENDSV
# error "ZenOS requires PendSV. Cortex-M0/M0+ (Armv6-M) has no PendSV; use a dedicated Armv6-M port/SysTick integration."
#endif

/* TrustZone/security state needs an explicit secure/non-secure port because
   EXC_RETURN, stack limits and MPU/SAU programming are security-sensitive. */
#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
# define OS_TRUSTZONE_BUILD 1
#else
# define OS_TRUSTZONE_BUILD 0
#endif

#if OS_TRUSTZONE_BUILD
# warning "ZenOS standard Cortex-M port is non-secure/single-domain. Provide a TrustZone-aware port before using secure/non-secure context switching."
#endif
