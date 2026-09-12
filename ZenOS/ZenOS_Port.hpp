#pragma once
/**
 * @file    ZenOS_Port.hpp
 * @brief   Platform-specific definitions for ARM Cortex-M
 *
 * Covers every STM32 family supported by ZenOS and every ARMv6-M/ARMv7-M/
 * ARMv8-M variant used in those families.  CMSIS device capability defines
 * (__ARM_ARCH_6M__, __ARM_ARCH_7M__, etc.) are authoritative for the core
 * architecture; the STM32 family macros provide peripheral-memory maps.
 *
 * @author  Rahman Heidari <rahman.h22@gmail.com> — Raymon Research Team
 * @version 2.0.0
 */

/* ═══════════════ Compiler / Linker Attributes ═══════════════ */
#define OS_NAKED    __attribute__((naked))
#define OS_USED     __attribute__((used))
#define OS_ALIGNED(n) __attribute__((aligned(n)))
#define OS_WEAK     __attribute__((weak))

/* ═══════════════ STM32 Family Detection ═══════════════
 * Each family maps to:
 *   OS_FAMILY_NAME      — human-readable string for banners
 *   OS_VECTOR_COUNT     — number of IRQs + 16 system exceptions
 *   OS_SMP_CORES        — number of cores (1 for all single-core STM32)
 *   OS_HAS_FPU_HW       — hardware FPU present on this specific chip
 *   OS_HAS_MPU_HW       — MPU hardware present
 *   OS_FLASH_SIZE       — typical flash size (bytes)
 *   OS_RAM_SIZE         — typical SRAM size (bytes)
 *   OS_CRC_BASE_ADDR    — CRC peripheral base address (varies by family)
 *   OS_IWDG_BASE_ADDR   — IWDG peripheral base address (varies by family)
 *   OS_RCC_CSR_ADDR     — RCC CSR register for reset flags
 *   OS_NVIC_PRIO_BITS   — number of implemented NVIC priority bits
 * ────────────────────────────────────────────────────────────
 * Vector counts are taken from the corresponding STM32 reference manuals.
 * For families with wide ranges (F2/F4/F7/H7), we use a mid-range value.
 * The actual VTOR size only matters when we copy the vector table in
 * os_start(); using the family's maximum is always safe. */

/* ── Cortex-M0 / M0+ (ARMv6-M) ──────────────────────
 * No BASEPRI, no MPU (M0) or optional MPU (M0+), Thumb-1 only.
 * These cores use a simpler exception model. */
#if defined(STM32F0xx)
    #define OS_FAMILY_NAME      "STM32F0"
    #define OS_VECTOR_COUNT     32
    #define OS_SMP_CORES        1
    #define OS_HAS_FPU_HW       0
    #define OS_HAS_MPU_HW       0
    #define OS_FLASH_SIZE       0x10000UL    /* 64KB (F030) */
    #define OS_RAM_SIZE         0x2000UL     /* 8KB */
    #define OS_NVIC_PRIO_BITS   2
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40021024UL

#elif defined(STM32G0xx)
    #define OS_FAMILY_NAME      "STM32G0"
    #define OS_VECTOR_COUNT     32
    #define OS_SMP_CORES        1
    #define OS_HAS_FPU_HW       0
    #define OS_HAS_MPU_HW       0
    #define OS_FLASH_SIZE       0x20000UL    /* 128KB (G070) */
    #define OS_RAM_SIZE         0x5000UL     /* 20KB */
    #define OS_NVIC_PRIO_BITS   2
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40021000UL /* G0 uses CR/CSR/BDCR */

/* ── Cortex-M3 (ARMv7-M) ──────────────────────────────
 * No FPU, optional MPU (M3), Thumb-2, BASEPRI available. */
#elif defined(STM32F1xx)
    #define OS_FAMILY_NAME      "STM32F1"
    #define OS_VECTOR_COUNT     68
    #define OS_SMP_CORES        1
    #define OS_HAS_FPU_HW       0
    #define OS_HAS_MPU_HW       1
    #define OS_FLASH_SIZE       0x10000UL    /* 64KB (F103C8) */
    #define OS_RAM_SIZE         0x5000UL     /* 20KB */
    #define OS_NVIC_PRIO_BITS   4
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40021024UL

#elif defined(STM32L0xx)
    #define OS_FAMILY_NAME      "STM32L0"
    #define OS_VECTOR_COUNT     32
    #define OS_SMP_CORES        1
    #define OS_HAS_FPU_HW       0
    #define OS_HAS_MPU_HW       1   /* L0 has optional MPU (Cortex-M0+) */
    #define OS_FLASH_SIZE       0x20000UL    /* 128KB (L072) */
    #define OS_RAM_SIZE         0x5000UL     /* 20KB */
    #define OS_NVIC_PRIO_BITS   2
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40021024UL

#elif defined(STM32L1xx)
    #define OS_FAMILY_NAME      "STM32L1"
    #define OS_VECTOR_COUNT     68
    #define OS_SMP_CORES        1
    #define OS_HAS_FPU_HW       0
    #define OS_HAS_MPU_HW       1   /* L1 has MPU (Cortex-M3) */
    #define OS_FLASH_SIZE       0x100000UL   /* 1MB (L152) */
    #define OS_RAM_SIZE         0x14000UL    /* 80KB */
    #define OS_NVIC_PRIO_BITS   4
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40023824UL

/* ── Cortex-M3 / Cortex-M4 (no FPU) ────────────────── */
#elif defined(STM32F2xx)
    #define OS_FAMILY_NAME      "STM32F2"
    #define OS_VECTOR_COUNT     81
    #define OS_SMP_CORES        1
    #define OS_HAS_FPU_HW       0
    #define OS_HAS_MPU_HW       1
    #define OS_FLASH_SIZE       0x100000UL   /* 1MB (F207) */
    #define OS_RAM_SIZE         0x20000UL    /* 128KB */
    #define OS_NVIC_PRIO_BITS   4
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40023824UL

#elif defined(STM32F3xx)
    #define OS_FAMILY_NAME      "STM32F3"
    #define OS_VECTOR_COUNT     82
    #define OS_SMP_CORES        1
    #define OS_HAS_FPU_HW       0   /* F303 has Cortex-M4 without FPU */
    #define OS_HAS_MPU_HW       1
    #define OS_FLASH_SIZE       0x40000UL    /* 256KB (F303) */
    #define OS_RAM_SIZE         0x10000UL    /* 64KB */
    #define OS_NVIC_PRIO_BITS   4
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40021024UL

/* ── Cortex-M4F (ARMv7E-M with FPU) ────────────────── */
#elif defined(STM32F4xx)
    #define OS_FAMILY_NAME      "STM32F4"
    #define OS_VECTOR_COUNT     86
    #define OS_SMP_CORES        1
    #define OS_HAS_FPU_HW       1   /* F407/F429 have single-precision FPU */
    #define OS_HAS_MPU_HW       1
    #define OS_FLASH_SIZE       0x200000UL   /* 2MB (F429) */
    #define OS_RAM_SIZE         0x20000UL    /* 128KB */
    #define OS_NVIC_PRIO_BITS   4
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40023824UL

#elif defined(STM32G4xx)
    #define OS_FAMILY_NAME      "STM32G4"
    #define OS_VECTOR_COUNT     100
    #define OS_SMP_CORES        1
    #define OS_HAS_FPU_HW       1   /* G431/G474 have single-precision FPU */
    #define OS_HAS_MPU_HW       1
    #define OS_FLASH_SIZE       0x80000UL    /* 512KB (G474) */
    #define OS_RAM_SIZE         0x18000UL    /* 96KB */
    #define OS_NVIC_PRIO_BITS   4
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40021024UL

/* ── Cortex-M7 (ARMv7E-M with FPU) ─────────────────── */
#elif defined(STM32F7xx)
    #define OS_FAMILY_NAME      "STM32F7"
    #define OS_VECTOR_COUNT     91
    #define OS_SMP_CORES        1
    #define OS_HAS_FPU_HW       1   /* F767/F746 have double-precision FPU */
    #define OS_HAS_MPU_HW       1
    #define OS_FLASH_SIZE       0x200000UL   /* 2MB (F767) */
    #define OS_RAM_SIZE         0x40000UL    /* 256KB */
    #define OS_NVIC_PRIO_BITS   4
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40023824UL

#elif defined(STM32H7xx)
    #define OS_FAMILY_NAME      "STM32H7"
    #define OS_VECTOR_COUNT     150
    #define OS_SMP_CORES        1
    #define OS_HAS_FPU_HW       1   /* H743/H750 have double-precision FPU */
    #define OS_HAS_MPU_HW       1
    #define OS_FLASH_SIZE       0x200000UL   /* 2MB (H743) */
    #define OS_RAM_SIZE         0x40000UL    /* 1MB total DTCM+AXI */
    #define OS_NVIC_PRIO_BITS   4
    #define OS_CRC_BASE_ADDR    0x58024C00UL /* H7 uses AHB4 bus */
    #define OS_IWDG_BASE_ADDR   0x58004800UL /* H7 uses APB4 bus */
    #define OS_RCC_CSR_ADDR     0x580244D0UL /* H7 uses RCC->CSR */

/* ── Cortex-M4F Low-Power ──────────────────────────── */
#elif defined(STM32L4xx)
    #define OS_FAMILY_NAME      "STM32L4"
    #define OS_VECTOR_COUNT     82
    #define OS_SMP_CORES        1
    #define OS_HAS_FPU_HW       1   /* L476/L496 have single-precision FPU */
    #define OS_HAS_MPU_HW       1
    #define OS_FLASH_SIZE       0x100000UL   /* 1MB (L496) */
    #define OS_RAM_SIZE         0x20000UL    /* 128KB */
    #define OS_NVIC_PRIO_BITS   4
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40021024UL

#elif defined(STM32L5xx)
    #define OS_FAMILY_NAME      "STM32L5"
    #define OS_VECTOR_COUNT     82
    #define OS_SMP_CORES        1
    #define OS_HAS_FPU_HW       1   /* L552 has single-precision FPU */
    #define OS_HAS_MPU_HW       1   /* L5 has TrustZone-aware MPU (PMSAv8) */
    #define OS_FLASH_SIZE       0x100000UL   /* 512KB */
    #define OS_RAM_SIZE         0x20000UL    /* 256KB */
    #define OS_NVIC_PRIO_BITS   4
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40021024UL

#elif defined(STM32U5xx)
    #define OS_FAMILY_NAME      "STM32U5"
    #define OS_VECTOR_COUNT     82
    #define OS_SMP_CORES        1
    #define OS_HAS_FPU_HW       1   /* U575/U585 have double-precision FPU */
    #define OS_HAS_MPU_HW       1   /* U5 has TrustZone-aware MPU (PMSAv8) */
    #define OS_FLASH_SIZE       0x200000UL   /* 2MB (U585) */
    #define OS_RAM_SIZE         0x20000UL    /* 256KB */
    #define OS_NVIC_PRIO_BITS   4
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40021024UL

#elif defined(STM32WBxx)
    #define OS_FAMILY_NAME      "STM32WB"
    #define OS_VECTOR_COUNT     66
    #define OS_SMP_CORES        1
    #define OS_HAS_FPU_HW       1   /* WB55 has single-precision FPU (Cortex-M4) */
    #define OS_HAS_MPU_HW       1
    #define OS_FLASH_SIZE       0x100000UL   /* 1MB (WB55) */
    #define OS_RAM_SIZE         0x40000UL    /* 256KB */
    #define OS_NVIC_PRIO_BITS   4
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40021024UL

#elif defined(STM32WBAxx)
    #define OS_FAMILY_NAME      "STM32WBA"
    #define OS_VECTOR_COUNT     82
    #define OS_SMP_CORES        1
    #define OS_HAS_FPU_HW       1   /* WBA52 has single-precision FPU (Cortex-M33) */
    #define OS_HAS_MPU_HW       1   /* M33 has PMSAv8 MPU */
    #define OS_FLASH_SIZE       0x200000UL   /* 1MB (WBA55) */
    #define OS_RAM_SIZE         0x40000UL    /* 256KB */
    #define OS_NVIC_PRIO_BITS   4
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40021024UL

/* ── Fallback: unknown STM32 or generic Cortex-M ──── */
#else
    #define OS_FAMILY_NAME      "Generic Cortex-M"
    #define OS_VECTOR_COUNT     68
    #define OS_SMP_CORES        1
    #if defined(__FPU_PRESENT) && (__FPU_PRESENT != 0U)
        #define OS_HAS_FPU_HW   1
    #else
        #define OS_HAS_FPU_HW   0
    #endif
    #if defined(__MPU_PRESENT) && (__MPU_PRESENT != 0U)
        #define OS_HAS_MPU_HW   1
    #else
        #define OS_HAS_MPU_HW   0
    #endif
    #define OS_FLASH_SIZE       0x20000UL
    #define OS_RAM_SIZE         0x5000UL
    #define OS_NVIC_PRIO_BITS   4
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40021024UL
#endif

#define OS_SMP_MAX_CORES 2

/* ═══════════════ Architecture Constants ═══════════════ */
#define OS_STACK_CANARY      0xDEADBEEFUL
#define OS_STACK_CANARY_COUNT 8
#define OS_TCB_MAGIC         0x54434200UL

#define OS_FLASH_START       0x08000000UL
#define OS_RAM_START         0x20000000UL

#define OS_RAM_TEST_SKIP     0x4000UL
#define OS_RAM_TEST_START    (OS_RAM_START + OS_RAM_TEST_SKIP)
#define OS_RAM_TEST_END      (OS_RAM_START + (OS_RAM_SIZE / 2))

/* ═══════════════ ARM Architecture Detection ═══════════════
 * CMSIS defines these based on the -mcpu flag:
 *   __ARM_ARCH_6M__   — Cortex-M0, M0+, M1 (ARMv6-M)
 *   __ARM_ARCH_7M__   — Cortex-M3 (ARMv7-M)
 *   __ARM_ARCH_7EM__  — Cortex-M4, M7 (ARMv7E-M)
 *   __ARM_ARCH_8M_BASE__ — Cortex-M23 (ARMv8-M Baseline)
 *   __ARM_ARCH_8M_MAIN__ — Cortex-M33 (ARMv8-M Mainline)
 */

/* MPU regions: architecture-dependent max */
#if defined(__ARM_ARCH_8M_MAIN__)
    #define OS_MPU_MAX_REGIONS  16   /* ARMv8-M Mainline: up to 16 regions */
#elif defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_BASE__)
    #define OS_MPU_MAX_REGIONS  8    /* ARMv7-M and ARMv8-M Baseline: 8 regions */
#else
    #define OS_MPU_MAX_REGIONS  8    /* safe default */
#endif

/* Architecture family string (used in banners and fault output) */
#if defined(__ARM_ARCH_6M__)
    #define OS_ARCH_NAME  "ARMv6-M (Cortex-M0/M0+)"
#elif defined(__ARM_ARCH_7M__)
    #define OS_ARCH_NAME  "ARMv7-M (Cortex-M3)"
#elif defined(__ARM_ARCH_7EM__)
    #define OS_ARCH_NAME  "ARMv7E-M (Cortex-M4/M7)"
#elif defined(__ARM_ARCH_8M_BASE__)
    #define OS_ARCH_NAME  "ARMv8-M Baseline (Cortex-M23)"
#elif defined(__ARM_ARCH_8M_MAIN__)
    #define OS_ARCH_NAME  "ARMv8-M Mainline (Cortex-M33)"
#else
    #define OS_ARCH_NAME  "ARM (unknown)"
#endif

/* ═══════════════ Vector Table ═══════════════ */
#define OS_HARDFAULT_VECTOR_INDEX   3
#define OS_MEMMANAGE_VECTOR_INDEX   4
#define OS_BUSFAULT_VECTOR_INDEX    5
#define OS_USAGEFAULT_VECTOR_INDEX  6
#define OS_PENDSV_VECTOR_INDEX      14
#define OS_SYSTICK_VECTOR_INDEX     15

/* ═══════════════ NVIC Priority Registers ═══════════════
 * The SHPR2/SHPR3 registers at 0xE000ED1C/0xE000ED20 control
 * the priority of system exceptions (SVCall, PendSV, SysTick). */
#define OS_SCB_BASE        0xE000ED00UL
#define OS_SCB_ICSR        (*((volatile uint32_t*)(OS_SCB_BASE + 0x04UL)))
#define OS_SCB_VTOR        (*((volatile uint32_t*)(OS_SCB_BASE + 0x08UL)))
#define OS_SCB_SHPR3       (*((volatile uint32_t*)(OS_SCB_BASE + 0x20UL)))
#define OS_ICSR_PENDSVSET_Msk (1UL << 28)

/* ═══════════════ SysTick Registers ═══════════════ */
#define OS_SYST_BASE       0xE000E010UL
#define OS_SYST_CSR        (*((volatile uint32_t*)(OS_SYST_BASE + 0x00UL)))
#define OS_SYST_RVR        (*((volatile uint32_t*)(OS_SYST_BASE + 0x04UL)))
#define OS_SYST_CVR        (*((volatile uint32_t*)(OS_SYST_BASE + 0x08UL)))
#define OS_SYST_CSR_ENABLE_Msk    (1UL << 0)
#define OS_SYST_CSR_TICKINT_Msk   (1UL << 1)
#define OS_SYST_CSR_COUNTFLAG_Msk (1UL << 16)

/* ═══════════════ DWT (Data Watchpoint and Trace) ═══════════════
 * Present on Cortex-M3+ except some Cortex-M0 variants.
 * Used for cycle-accurate µs timing via DWT->CYCCNT. */
#define OS_DWT_BASE        0xE0001000UL
#define OS_DWT_CTRL        (*((volatile uint32_t*)(OS_DWT_BASE + 0x00UL)))
#define OS_DWT_CYCCNT      (*((volatile uint32_t*)(OS_DWT_BASE + 0x04UL)))
#define OS_DWT_CTRL_CYCCNTENA_Msk (1UL << 0)
#define OS_COREDEBUG_DEMCR (*(volatile uint32_t*)0xE000EDFCUL)
#define OS_COREDEM_TRCENA  (1UL << 24)
#define OS_DWT_CYCCNTENA   (1UL << 0)

/* Cycle counter detection: CMSIS defines DWT on most Cortex-M3+.
   We also check known STM32 families that always have it. */
#if defined(DWT) || defined(DWT_LSR)
    #define OS_HAS_CYCLE_COUNTER 1
#elif defined(STM32F1xx) || defined(STM32F2xx) || defined(STM32F3xx) || \
      defined(STM32F4xx) || defined(STM32F7xx) || defined(STM32H7xx) || \
      defined(STM32G4xx) || defined(STM32L4xx) || defined(STM32L1xx) || \
      defined(STM32L5xx) || defined(STM32U5xx) || defined(STM32WBxx) || \
      defined(STM32WBAxx)
    #define OS_HAS_CYCLE_COUNTER 1
#elif defined(STM32F0xx) || defined(STM32G0xx) || defined(STM32L0xx)
    /* Cortex-M0/M0+: no DWT CYCCNT */
    #define OS_HAS_CYCLE_COUNTER 0
#else
    /* Try to use CMSIS detection */
    #define OS_HAS_CYCLE_COUNTER 0
#endif

/* ═══════════════ Hardware Feature Overrides ═══════════════
 * The family-level flags above give per-chip defaults.
 * CMSIS __MPU_PRESENT etc. can override for non-standard configs. */

/* MPU: trust CMSIS device header if available, else use family default */
#if defined(__MPU_PRESENT)
    #if (__MPU_PRESENT == 0U)
        #undef  OS_HAS_MPU_HW
        #define OS_HAS_MPU_HW 0
    #endif
#else
    /* No CMSIS header: fall back to family-level default */
#endif

/* VTOR: present on all ARMv7-M+ (Cortex-M3+), absent on ARMv6-M (M0/M0+) */
#if defined(__VTOR_PRESENT)
    #define OS_HAS_VTOR ((__VTOR_PRESENT) != 0U)
#elif defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_8M_BASE__)
    #define OS_HAS_VTOR 0
#else
    #define OS_HAS_VTOR 1
#endif

/* FPU: trust CMSIS device header if available */
#if defined(__FPU_PRESENT)
    #define OS_HAS_FPU_HW ((__FPU_PRESENT) != 0U)
#elif !defined(OS_HAS_FPU_HW)
    #define OS_HAS_FPU_HW 0
#endif

/* ═══════════════ Cortex-M Architecture Selection ═══════════════
 * Defines the core architecture class for context switch and MPU code.
 * These are exclusive — exactly one will be defined. */
#if defined(__ARM_ARCH_6M__)
    #define OS_ARCH_ARMV6M  1   /* Cortex-M0, M0+ */
    #define OS_ARCH_ARMV7M  0
    #define OS_ARCH_ARMV7EM 0
    #define OS_ARCH_ARMV8M  0
#elif defined(__ARM_ARCH_8M_BASE__)
    #define OS_ARCH_ARMV6M  0
    #define OS_ARCH_ARMV7M  0
    #define OS_ARCH_ARMV7EM 0
    #define OS_ARCH_ARMV8M  1   /* Cortex-M23 */
#elif defined(__ARM_ARCH_7M__)
    #define OS_ARCH_ARMV6M  0
    #define OS_ARCH_ARMV7M  1   /* Cortex-M3 */
    #define OS_ARCH_ARMV7EM 0
    #define OS_ARCH_ARMV8M  0
#elif defined(__ARM_ARCH_7EM__)
    #define OS_ARCH_ARMV6M  0
    #define OS_ARCH_ARMV7M  0
    #define OS_ARCH_ARMV7EM 1   /* Cortex-M4, M7 */
    #define OS_ARCH_ARMV8M  0
#else
    /* Unknown: default to ARMv7-M safe path */
    #define OS_ARCH_ARMV6M  0
    #define OS_ARCH_ARMV7M  1
    #define OS_ARCH_ARMV7EM 0
    #define OS_ARCH_ARMV8M  0
#endif

/* ═══════════════ IWDG Peripheral ═══════════════
 * Different STM32 families put IWDG on different buses:
 *   STM32F1/F2/F3/F4:  APB1 @ 0x40003000
 *   STM32F7/L4/L5/WB:  APB1 @ 0x40003000
 *   STM32H7:            APB4 @ 0x58004800
 * The IWDG-KR register offset is always +0x00.
 * The IWDG-SR register offset is always +0x0C. */
#ifndef OS_IWDG_BASE_ADDR
    #define OS_IWDG_BASE_ADDR  0x40003000UL
#endif
#define OS_IWDG_KR  (*((volatile uint32_t*)(OS_IWDG_BASE_ADDR + 0x00UL)))
#define OS_IWDG_SR  (*((volatile uint32_t*)(OS_IWDG_BASE_ADDR + 0x0CUL)))

/* ═══════════════ CRC Peripheral ═══════════════
 * CRC peripheral base addresses vary significantly across families:
 *   STM32F1/F2/F3/F4/F7/L0/L1/L4/WB/G4:  0x40023000
 *   STM32H7:                                0x58024C00 (AHB4)
 *   STM32L5/U5/WBA:                         0x40023000
 * The CRC-DR register offset is always +0x00.
 * The CRC-CR register offset is always +0x08. */
#ifndef OS_CRC_BASE_ADDR
    #define OS_CRC_BASE_ADDR  0x40023000UL
#endif
#define OS_CRC_DR  (*((volatile uint32_t*)(OS_CRC_BASE_ADDR + 0x00UL)))
#define OS_CRC_CR  (*((volatile uint32_t*)(OS_CRC_BASE_ADDR + 0x08UL)))

/* ═══════════════ NVIC Priority Helper ═══════════════
 * Different STM32 families implement different numbers of NVIC priority bits
 * (2, 3, or 4).  CMSIS defines __NVIC_PRIO_BITS from the device header.
 * The PendSV and SysTick priorities must be set to the lowest priority
 * (all bits set) so they never preempt any application interrupt. */
#ifndef OS_NVIC_PRIO_BITS
    #if defined(__NVIC_PRIO_BITS)
        #define OS_NVIC_PRIO_BITS __NVIC_PRIO_BITS
    #elif defined(OS_NVIC_PRIO_BITS)
        /* Use the family-level default set above */
    #else
        #define OS_NVIC_PRIO_BITS 4
    #endif
#endif

#define OS_PENDSV_PRIO    (*((volatile uint8_t*)0xE000ED22UL))
#define OS_SYSTICK_PRIO   (*((volatile uint8_t*)0xE000ED23UL))

/* ═══════════════ MPU Type Definitions ═══════════════
 * Different ARM architectures use different MPU register layouts:
 *
 * ARMv7-M (Cortex-M3/M4/M7): PMSAv7
 *   - RBAR: Region Base Address Register (region number in bits [3:0])
 *   - RASR: Region Attribute and Size Register (AP, TEX, S, C, B, XN, SIZE)
 *   - 8 regions max
 *
 * ARMv8-M (Cortex-M23/M33): PMSAv8
 *   - RBAR: Region Base Address Register (region number in RBAR[3:0])
 *   - RLAR: Region Limit Address Register (LIMIT, AttrIndx, EN, SH, AP, XN)
 *   - Up to 16 regions on Mainline
 *
 * The MPU implementation in ZenOS_Safety.cpp uses the ARMv7-M layout
 * by default.  When targeting ARMv8-M (M23/M33), enable OS_MPU_PMSAv8
 * in your project defines or let the architecture detection set it. */
#if defined(__ARM_ARCH_8M_MAIN__) || defined(__ARM_ARCH_8M_BASE__)
    #define OS_MPU_PMSAv8  1
    #define OS_MPU_PMSAv7  0
    /* PMSAv8 MPU register layout */
    typedef struct {
        volatile uint32_t CTRL;
        volatile uint32_t RNR;
        volatile uint32_t RBAR;
        volatile uint32_t RLAR;   /* PMSAv8 uses RLAR, not RASR */
        volatile uint32_t RBAR_Alt;
        volatile uint32_t RLAR_Alt;
        volatile uint32_t SFSR;
        volatile uint32_t SFAR;
    } OS_MPU_Type;
#else
    #define OS_MPU_PMSAv8  0
    #define OS_MPU_PMSAv7  1
    /* PMSAv7 MPU register layout */
    typedef struct {
        volatile uint32_t CTRL;
        volatile uint32_t RNR;
        volatile uint32_t RBAR;
        volatile uint32_t RASR;
    } OS_MPU_Type;
#endif
#define OS_MPU_BASE ((OS_MPU_Type*)0xE000ED90UL)

/* ═══════════════ Cache Operations (Cortex-M7/H7) ═══════════════
 * Cortex-M7 and H7 have data/instruction caches that can cause
 * coherency issues during context switch if not handled.
 * SCB->CCR (Configuration and Control Register) bits:
 *   [16] IC: Instruction cache enable
 *   [17] DC: Data cache enable
 * These are typically enabled by the HAL/SystemInit.  The context
 * switch does NOT need to flush caches because:
 *   - All writes to TCB/stack go through normal memory (not cached region)
 *   - Cortex-M7 SCB->CCR can be configured to bypass cache for SHARED regions
 *   - Cortex-M7 CMSIS provides SCB_CleanDCache_by_Addr() for explicit flush
 *
 * If you enable cache for SRAM regions used by the kernel, you MUST
 * either:
 *   (a) Set the SRAM region as non-cacheable via MPU, or
 *   (b) Call SCB_CleanDCache_by_Addr() after modifying TCBs.
 * This is a porting concern — see PORTING_GUIDE.md. */
#define OS_HAS_CACHE  0  /* Default: assume no cache. Set to 1 in project
                            defines if targeting STM32F7/H7 with cache enabled. */
