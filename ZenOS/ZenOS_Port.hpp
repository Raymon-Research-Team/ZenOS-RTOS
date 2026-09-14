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
 * @version 1.1.0
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
    #define OS_RCC_CSR_ADDR     0x40021094UL /* G0 RCC_CSR @ base 0x40021000 + 0x94 */

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
    #define OS_RAM_SIZE         0x40000UL    /* 256KB typical (H743 = 1MB across DTCM+AXI+SRAM1-4) */
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

/* ── Fallback: unknown STM32 or generic Cortex-M ────
 * Uses __ARM_ARCH_* (set by -mcpu flag, NOT CMSIS) for architecture
 * features. No CMSIS device header dependency. */
#else
    #define OS_FAMILY_NAME      "Generic Cortex-M"
    #define OS_VECTOR_COUNT     68
    #define OS_SMP_CORES        1
    /* FPU: ARMv7E-M+ (M4F/M7) always has FPU; ARMv7-M (M3) and ARMv6-M (M0/M0+) do not */
    #if defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_MAIN__)
        #define OS_HAS_FPU_HW   1
    #else
        #define OS_HAS_FPU_HW   0
    #endif
    /* MPU: ARMv7-M+ has MPU (optional on M0+); ARMv6-M (M0) does not */
    #if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || \
        defined(__ARM_ARCH_8M_BASE__) || defined(__ARM_ARCH_8M_MAIN__)
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

/* Now that OS_CRC_BASE_ADDR and OS_IWDG_BASE_ADDR are set per-family,
   include ZenOS_Regs.hpp which defines the register-level accessors
   (OS_CRC_DR, OS_CRC_CR, OS_IWDG_KR, OS_IWDG_SR) using those addresses. */
#include "ZenOS_Regs.hpp"

#define OS_SMP_MAX_CORES 2

/* ═══════════════ Debug UART Peripheral Mapping ═══════════════
 * Each STM32 family uses different GPIO AF mechanisms:
 *   F0/F1/F3: AFIO remap (MAPR register) — legacy style
 *   F2/F4/F7/H7/G0/G4/L0/L1/L4/L5/U5/WB/WBA: MODER + AFR — modern style
 *
 * OS_DEBUG_USART_DEFAULT_BASE: USART peripheral base address for debug
 * OS_DEBUG_GPIO_DEFAULT_BASE:  GPIO base for TX pin
 * OS_DEBUG_GPIO_MODER_OFFSET:  MODER register offset (modern families)
 * OS_DEBUG_GPIO_AFR_OFFSET:    AFR register offset (modern families)
 * OS_DEBUG_RCC_ENR:            RCC peripheral clock enable register
 * OS_DEBUG_RCC_ENR_BIT:        Bit position in RCC enable register
 *
 * These are defaults — override all via project defines.
 * Ref: STM32 Reference Manual, GPIO and RCC chapters per family.
 */

/* STM32F1 — AFIO remap style (legacy GPIO config via CRL/CRH) */
#if defined(STM32F1xx)
    #ifndef OS_DEBUG_USART_DEFAULT_BASE
    #define OS_DEBUG_USART_DEFAULT_BASE  0x40013800UL  /* USART1 @ APB2 */
    #endif
    #ifndef OS_DEBUG_GPIO_DEFAULT_BASE
    #define OS_DEBUG_GPIO_DEFAULT_BASE   0x40010800UL  /* GPIOA @ APB2 */
    #endif
    /* F1: RCC->APB2ENR, USART1EN=bit14, IOPAEN=bit2 */
    #ifndef OS_DEBUG_RCC_ENR
    #define OS_DEBUG_RCC_ENR        0x40021018UL       /* RCC->APB2ENR */
    #endif
    #ifndef OS_DEBUG_RCC_USART_BIT
    #define OS_DEBUG_RCC_USART_BIT  14U
    #endif
    #ifndef OS_DEBUG_RCC_GPIO_BIT
    #define OS_DEBUG_RCC_GPIO_BIT   2U
    #endif

/* STM32F0 — MODER style (Cortex-M0, no AFIO) */
#elif defined(STM32F0xx)
    #ifndef OS_DEBUG_USART_DEFAULT_BASE
    #define OS_DEBUG_USART_DEFAULT_BASE  0x40013800UL  /* USART1 */
    #endif
    #ifndef OS_DEBUG_GPIO_DEFAULT_BASE
    #define OS_DEBUG_GPIO_DEFAULT_BASE   0x48000000UL  /* GPIOA */
    #endif
    #ifndef OS_DEBUG_RCC_ENR
    #define OS_DEBUG_RCC_ENR        0x40021014UL       /* RCC->APB2ENR */
    #endif
    #ifndef OS_DEBUG_RCC_USART_BIT
    #define OS_DEBUG_RCC_USART_BIT  14U
    #endif
    #ifndef OS_DEBUG_RCC_GPIO_BIT
    #define OS_DEBUG_RCC_GPIO_BIT   17U               /* IOPAEN in AHBENR */
    #endif

/* STM32F2/F4 — MODER + AFR style */
#elif defined(STM32F2xx) || defined(STM32F4xx)
    #ifndef OS_DEBUG_USART_DEFAULT_BASE
    #define OS_DEBUG_USART_DEFAULT_BASE  0x40011000UL  /* USART1 @ APB2 */
    #endif
    #ifndef OS_DEBUG_GPIO_DEFAULT_BASE
    #define OS_DEBUG_GPIO_DEFAULT_BASE   0x40020000UL  /* GPIOA @ AHB1 */
    #endif
    #ifndef OS_DEBUG_RCC_ENR
    #define OS_DEBUG_RCC_ENR        0x40023830UL       /* RCC->APB2ENR */
    #endif
    #ifndef OS_DEBUG_RCC_USART_BIT
    #define OS_DEBUG_RCC_USART_BIT  4U                 /* USART1EN */
    #endif
    #ifndef OS_DEBUG_RCC_GPIO_BIT
    #define OS_DEBUG_RCC_GPIO_BIT   0U                 /* GPIOAEN in AHB1ENR */
    #endif

/* STM32F3 — MODER + AFR style */
#elif defined(STM32F3xx)
    #ifndef OS_DEBUG_USART_DEFAULT_BASE
    #define OS_DEBUG_USART_DEFAULT_BASE  0x40013800UL  /* USART1 */
    #endif
    #ifndef OS_DEBUG_GPIO_DEFAULT_BASE
    #define OS_DEBUG_GPIO_DEFAULT_BASE   0x48000000UL  /* GPIOA */
    #endif
    #ifndef OS_DEBUG_RCC_ENR
    #define OS_DEBUG_RCC_ENR        0x40021014UL       /* RCC->APB2ENR */
    #endif
    #ifndef OS_DEBUG_RCC_USART_BIT
    #define OS_DEBUG_RCC_USART_BIT  14U
    #endif
    #ifndef OS_DEBUG_RCC_GPIO_BIT
    #define OS_DEBUG_RCC_GPIO_BIT   17U                /* IOPAEN in AHBENR */
    #endif

/* STM32F7 — MODER + AFR style */
#elif defined(STM32F7xx)
    #ifndef OS_DEBUG_USART_DEFAULT_BASE
    #define OS_DEBUG_USART_DEFAULT_BASE  0x40011000UL  /* USART1 */
    #endif
    #ifndef OS_DEBUG_GPIO_DEFAULT_BASE
    #define OS_DEBUG_GPIO_DEFAULT_BASE   0x40020000UL  /* GPIOA */
    #endif
    #ifndef OS_DEBUG_RCC_ENR
    #define OS_DEBUG_RCC_ENR        0x40023830UL       /* RCC->APB2ENR */
    #endif
    #ifndef OS_DEBUG_RCC_USART_BIT
    #define OS_DEBUG_RCC_USART_BIT  4U
    #endif
    #ifndef OS_DEBUG_RCC_GPIO_BIT
    #define OS_DEBUG_RCC_GPIO_BIT   0U                 /* GPIOAEN in AHB1ENR */
    #endif

/* STM32G0 — MODER + AFR style (Cortex-M0+) */
#elif defined(STM32G0xx)
    #ifndef OS_DEBUG_USART_DEFAULT_BASE
    #define OS_DEBUG_USART_DEFAULT_BASE  0x40013800UL  /* USART1 */
    #endif
    #ifndef OS_DEBUG_GPIO_DEFAULT_BASE
    #define OS_DEBUG_GPIO_DEFAULT_BASE   0x50000000UL  /* GPIOA */
    #endif
    #ifndef OS_DEBUG_RCC_ENR
    #define OS_DEBUG_RCC_ENR        0x40021040UL       /* RCC->APBENR2 */
    #endif
    #ifndef OS_DEBUG_RCC_USART_BIT
    #define OS_DEBUG_RCC_USART_BIT  14U
    #endif
    #ifndef OS_DEBUG_RCC_GPIO_BIT
    #define OS_DEBUG_RCC_GPIO_BIT   0U                 /* GPIOAEN in IOPENR */
    #endif

/* STM32G4 — MODER + AFR style */
#elif defined(STM32G4xx)
    #ifndef OS_DEBUG_USART_DEFAULT_BASE
    #define OS_DEBUG_USART_DEFAULT_BASE  0x40013800UL  /* USART1 */
    #endif
    #ifndef OS_DEBUG_GPIO_DEFAULT_BASE
    #define OS_DEBUG_GPIO_DEFAULT_BASE   0x48000000UL  /* GPIOA */
    #endif
    #ifndef OS_DEBUG_RCC_ENR
    #define OS_DEBUG_RCC_ENR        0x40021060UL       /* RCC->APB2ENR */
    #endif
    #ifndef OS_DEBUG_RCC_USART_BIT
    #define OS_DEBUG_RCC_USART_BIT  4U
    #endif
    #ifndef OS_DEBUG_RCC_GPIO_BIT
    #define OS_DEBUG_RCC_GPIO_BIT   0U                 /* GPIOAEN in AHB2ENR */
    #endif

/* STM32H7 — MODER + AFR style (different bus) */
#elif defined(STM32H7xx)
    #ifndef OS_DEBUG_USART_DEFAULT_BASE
    #define OS_DEBUG_USART_DEFAULT_BASE  0x40011000UL  /* USART1 @ APB2 */
    #endif
    #ifndef OS_DEBUG_GPIO_DEFAULT_BASE
    #define OS_DEBUG_GPIO_DEFAULT_BASE   0x58020000UL  /* GPIOA @ AHB4 */
    #endif
    #ifndef OS_DEBUG_RCC_ENR
    #define OS_DEBUG_RCC_ENR        0x58024460UL       /* RCC->APB2ENR */
    #endif
    #ifndef OS_DEBUG_RCC_USART_BIT
    #define OS_DEBUG_RCC_USART_BIT  4U
    #endif
    #ifndef OS_DEBUG_RCC_GPIO_BIT
    #define OS_DEBUG_RCC_GPIO_BIT   0U                 /* GPIOAEN in AHB4ENR */
    #endif

/* STM32L0 — MODER style (Cortex-M0+) */
#elif defined(STM32L0xx)
    #ifndef OS_DEBUG_USART_DEFAULT_BASE
    #define OS_DEBUG_USART_DEFAULT_BASE  0x40011400UL  /* USART2 (L0 often uses USART2) */
    #endif
    #ifndef OS_DEBUG_GPIO_DEFAULT_BASE
    #define OS_DEBUG_GPIO_DEFAULT_BASE   0x50000000UL  /* GPIOA */
    #endif
    #ifndef OS_DEBUG_RCC_ENR
    #define OS_DEBUG_RCC_ENR        0x40021028UL       /* RCC->APB1ENR */
    #endif
    #ifndef OS_DEBUG_RCC_USART_BIT
    #define OS_DEBUG_RCC_USART_BIT  17U                /* USART2EN */
    #endif
    #ifndef OS_DEBUG_RCC_GPIO_BIT
    #define OS_DEBUG_RCC_GPIO_BIT   0U                 /* IOPAEN in IOPENR */
    #endif

/* STM32L1 — MODER style (Cortex-M3) */
#elif defined(STM32L1xx)
    #ifndef OS_DEBUG_USART_DEFAULT_BASE
    #define OS_DEBUG_USART_DEFAULT_BASE  0x40013800UL  /* USART1 */
    #endif
    #ifndef OS_DEBUG_GPIO_DEFAULT_BASE
    #define OS_DEBUG_GPIO_DEFAULT_BASE   0x40020000UL  /* GPIOA */
    #endif
    #ifndef OS_DEBUG_RCC_ENR
    #define OS_DEBUG_RCC_ENR        0x40023824UL       /* RCC->APB2ENR */
    #endif
    #ifndef OS_DEBUG_RCC_USART_BIT
    #define OS_DEBUG_RCC_USART_BIT  14U
    #endif
    #ifndef OS_DEBUG_RCC_GPIO_BIT
    #define OS_DEBUG_RCC_GPIO_BIT   0U                 /* IOPAEN in AHBENR */
    #endif

/* STM32L4/L5/U5 — MODER + AFR style */
#elif defined(STM32L4xx) || defined(STM32L5xx) || defined(STM32U5xx)
    #ifndef OS_DEBUG_USART_DEFAULT_BASE
    #define OS_DEBUG_USART_DEFAULT_BASE  0x40013800UL  /* USART1 */
    #endif
    #ifndef OS_DEBUG_GPIO_DEFAULT_BASE
    #define OS_DEBUG_GPIO_DEFAULT_BASE   0x48000000UL  /* GPIOA */
    #endif
    #ifndef OS_DEBUG_RCC_ENR
    #define OS_DEBUG_RCC_ENR        0x40021060UL       /* RCC->APB2ENR */
    #endif
    #ifndef OS_DEBUG_RCC_USART_BIT
    #define OS_DEBUG_RCC_USART_BIT  4U
    #endif
    #ifndef OS_DEBUG_RCC_GPIO_BIT
    #define OS_DEBUG_RCC_GPIO_BIT   0U                 /* GPIOAEN in AHB2ENR */
    #endif

/* STM32WB — MODER + AFR style */
#elif defined(STM32WBxx)
    #ifndef OS_DEBUG_USART_DEFAULT_BASE
    #define OS_DEBUG_USART_DEFAULT_BASE  0x40013800UL  /* USART1 */
    #endif
    #ifndef OS_DEBUG_GPIO_DEFAULT_BASE
    #define OS_DEBUG_GPIO_DEFAULT_BASE   0x48000000UL  /* GPIOA */
    #endif
    #ifndef OS_DEBUG_RCC_ENR
    #define OS_DEBUG_RCC_ENR        0x58000058UL       /* RCC->APB2ENR */
    #endif
    #ifndef OS_DEBUG_RCC_USART_BIT
    #define OS_DEBUG_RCC_USART_BIT  4U
    #endif
    #ifndef OS_DEBUG_RCC_GPIO_BIT
    #define OS_DEBUG_RCC_GPIO_BIT   0U                 /* GPIOAEN in AHB2ENR */
    #endif

/* STM32WBA — MODER + AFR style */
#elif defined(STM32WBAxx)
    #ifndef OS_DEBUG_USART_DEFAULT_BASE
    #define OS_DEBUG_USART_DEFAULT_BASE  0x40013800UL  /* USART1 */
    #endif
    #ifndef OS_DEBUG_GPIO_DEFAULT_BASE
    #define OS_DEBUG_GPIO_DEFAULT_BASE   0x48000000UL  /* GPIOA */
    #endif
    #ifndef OS_DEBUG_RCC_ENR
    #define OS_DEBUG_RCC_ENR        0x58000098UL       /* RCC->APB2ENR */
    #endif
    #ifndef OS_DEBUG_RCC_USART_BIT
    #define OS_DEBUG_RCC_USART_BIT  4U
    #endif
    #ifndef OS_DEBUG_RCC_GPIO_BIT
    #define OS_DEBUG_RCC_GPIO_BIT   0U
    #endif

/* Generic fallback */
#else
    #ifndef OS_DEBUG_USART_DEFAULT_BASE
    #define OS_DEBUG_USART_DEFAULT_BASE  0x40013800UL
    #endif
    #ifndef OS_DEBUG_GPIO_DEFAULT_BASE
    #define OS_DEBUG_GPIO_DEFAULT_BASE   0x40010800UL
    #endif
    #ifndef OS_DEBUG_RCC_ENR
    #define OS_DEBUG_RCC_ENR        0x40021018UL
    #endif
    #ifndef OS_DEBUG_RCC_USART_BIT
    #define OS_DEBUG_RCC_USART_BIT  14U
    #endif
    #ifndef OS_DEBUG_RCC_GPIO_BIT
    #define OS_DEBUG_RCC_GPIO_BIT   2U
    #endif
#endif

/* GPIO register offsets (modern families: MODER/OTYPER/OSPEEDR/PUPDR/ODR/AFR) */
/* Ref: STM32 RM, GPIO chapter — offsets are identical across all STM32 families */
#define OS_GPIO_MODER_OFFSET   0x00UL
#define OS_GPIO_OTYPER_OFFSET  0x04UL
#define OS_GPIO_OSPEEDR_OFFSET 0x08UL
#define OS_GPIO_PUPDR_OFFSET   0x0CUL
#define OS_GPIO_ODR_OFFSET     0x14UL
#define OS_GPIO_BSRR_OFFSET    0x18UL
#define OS_GPIO_AFRL_OFFSET    0x20UL  /* AFR[0] for pins 0-7 */
#define OS_GPIO_AFRH_OFFSET    0x24UL  /* AFR[1] for pins 8-15 */

/* ═══════════════ RCC Reset-Flag Bit Positions ═══════════════
 * os_get_hw_wdg_reset_count() must read the IWDG reset flag and clear
 * all reset flags using the RCC_CSR layout of the *current* family.  The
 * bit positions are NOT identical across STM32 series:
 *
 *   Family group          IWDGRSTF  RMVF
 *   F0/F1/F2/F3/F4/F7/L0/L1/L4/L5/U5/WB/WBA/G4   29   24
 *   G0/L5(alt)/U5(alt)    -- see RM (G0 CSR @ +0x94, bits differ)
 *   H7                    -- RCC_CSR layout differs (bit 29 still IWDGRSTF)
 *
 * Default to the classic 29/24 pair (correct for the vast majority of
 * families) and allow a project to override with -D. */
#ifndef OS_RCC_IWDGRSTF_BIT
    #define OS_RCC_IWDGRSTF_BIT   29U
#endif
#ifndef OS_RCC_RMVF_BIT
    #define OS_RCC_RMVF_BIT       24U
#endif

/* ═══════════════ Architecture Constants ═══════════════ */
#define OS_STACK_CANARY      0xDEADBEEFUL
#define OS_STACK_CANARY_COUNT 8
#define OS_TCB_MAGIC         0x54434200UL

#define OS_FLASH_START       0x08000000UL
#define OS_RAM_START         0x20000000UL

#define OS_RAM_TEST_SKIP     0x4000UL
#define OS_RAM_TEST_START    (OS_RAM_START + OS_RAM_TEST_SKIP)
/* RAM test covers the lower half of RAM. The upper half typically contains
   active stack and heap — a destructive March-C test there would corrupt
   live data. For IEC 62304 Class C / IEC 61508 SIL 3+ full coverage,
   either:
   (a) Run a non-destructive read-only pattern check on the upper half, or
   (b) Schedule the test during a safe shutdown window.
   Override with -DOS_RAM_TEST_END=<addr> to extend coverage. */
#ifndef OS_RAM_TEST_END
#define OS_RAM_TEST_END      (OS_RAM_START + (OS_RAM_SIZE / 2))
#endif

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

/* NVIC Priority, SysTick, DWT, SCB register definitions are now in
   ZenOS_Regs.hpp (included above). Do NOT redefine them here. */

/* Cycle counter detection: DWT CYCCNT is present on Cortex-M3+ (ARMv7-M).
   Cortex-M0/M0+ (ARMv6-M) does NOT have CYCCNT.
   Use known STM32 families + __ARM_ARCH_* for detection. */
#if defined(STM32F0xx) || defined(STM32G0xx) || defined(STM32L0xx)
    /* Cortex-M0/M0+: no DWT CYCCNT */
    #define OS_HAS_CYCLE_COUNTER 0
#elif defined(__ARM_ARCH_6M__)
    /* ARMv6-M: no DWT CYCCNT */
    #define OS_HAS_CYCLE_COUNTER 0
#elif defined(STM32F1xx) || defined(STM32F2xx) || defined(STM32F3xx) || \
      defined(STM32F4xx) || defined(STM32F7xx) || defined(STM32H7xx) || \
      defined(STM32G4xx) || defined(STM32L4xx) || defined(STM32L1xx) || \
      defined(STM32L5xx) || defined(STM32U5xx) || defined(STM32WBxx) || \
      defined(STM32WBAxx)
    #define OS_HAS_CYCLE_COUNTER 1
#elif defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || \
      defined(__ARM_ARCH_8M_BASE__) || defined(__ARM_ARCH_8M_MAIN__)
    #define OS_HAS_CYCLE_COUNTER 1
#else
    #define OS_HAS_CYCLE_COUNTER 0
#endif

/* ═══════════════ Hardware Feature Detection (CMSIS-Free) ═══════════════
 * All feature flags are derived from:
 *   - STM32Fxxx/Gxxx/Lxxx/Hxxx family macros (set by CubeMX or project)
 *   - __ARM_ARCH_* macros (set by -mcpu compiler flag)
 * NO dependency on CMSIS device headers (__MPU_PRESENT, __FPU_PRESENT,
 * __VTOR_PRESENT, __NVIC_PRIO_BITS). */

/* VTOR: present on all ARMv7-M+ (Cortex-M3+), absent on ARMv6-M (M0/M0+).
 * ARMv8-M Baseline (Cortex-M23) DOES have VTOR. */
#if defined(__ARM_ARCH_6M__)
    #define OS_HAS_VTOR 0
#else
    #define OS_HAS_VTOR 1
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
/* IWDG register definitions are now in ZenOS_Regs.hpp.
   The OS_IWDG_BASE_ADDR is set per-family above; ZenOS_Regs.hpp
   provides OS_IWDG_KR / OS_IWDG_SR using that address. */

/* ═══════════════ CRC Peripheral ═══════════════
 * CRC peripheral base addresses vary significantly across families:
 *   STM32F1/F2/F3/F4/F7/L0/L1/L4/WB/G4:  0x40023000
 *   STM32H7:                                0x58024C00 (AHB4)
 *   STM32L5/U5/WBA:                         0x40023000
 * The CRC-DR register offset is always +0x00.
 * The CRC-CR register offset is always +0x08. */
/* CRC register definitions are now in ZenOS_Regs.hpp.
   The OS_CRC_BASE_ADDR is set per-family above; ZenOS_Regs.hpp
   provides OS_CRC_DR / OS_CRC_CR using that address. */

/* ═══════════════ NVIC Priority Helper ═══════════════
 * Different STM32 families implement different numbers of NVIC priority bits
 * (2, 3, or 4).  OS_NVIC_PRIO_BITS is set per-family above (ZenOS_Port.hpp)
 * and the PendSV/SysTick priority registers are in ZenOS_Regs.hpp.
 * The PendSV and SysTick priorities must be set to the lowest priority
 * (all bits set) so they never preempt any application interrupt. */
/* OS_NVIC_PRIO_BITS: already defined per-family in the STM32 detection
   block above.  Fallback to 4 if nothing matched (generic Cortex-M). */
#ifndef OS_NVIC_PRIO_BITS
    #define OS_NVIC_PRIO_BITS 4
#endif

/* MPU type definitions are now in ZenOS_Regs.hpp (included above).
   Do NOT redefine OS_MPU_Type or OS_MPU_BASE here. */

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

/* ═══════════════ CubeMX Peripheral Consistency Checks ═══════════════
 * If CubeMX enables a peripheral (via HAL_IWDG_MODULE_ENABLED, etc.)
 * but the corresponding ZenOS safety feature is disabled, the peripheral
 * runs unsupervised — a safety hazard.
 *
 * These checks use HAL_*_MODULE_ENABLED defines which are available when:
 *   - CubeMX project includes HAL headers (stm32f1xx_hal_conf.h), OR
 *   - User defines HAL_IWDG_MODULE_ENABLED in project preprocessor defines
 *
 * They are #ifdef-guarded so they are harmless when HAL is not used.
 * If you get a #error from these checks, either:
 *   (a) Enable the matching OS_SAFETY_* in ZenOS_Config.hpp, OR
 *   (b) Disable the peripheral in CubeMX .ioc file
 */
#if defined(HAL_IWDG_MODULE_ENABLED) && !OS_SAFETY_HW_WATCHDOG
#error "[ZenOS] IWDG enabled in CubeMX but OS_SAFETY_HW_WATCHDOG=0. \
Enable OS_SAFETY_HW_WATCHDOG=1 or disable IWDG in CubeMX. \
Running IWDG without kernel-managed feed causes spurious MCU resets."
#endif

#if defined(HAL_CRC_MODULE_ENABLED) && !OS_SAFETY_CRC_CHECK
#error "[ZenOS] CRC enabled in CubeMX but OS_SAFETY_CRC_CHECK=0. \
Enable OS_SAFETY_CRC_CHECK=1 or disable CRC in CubeMX."
#endif
