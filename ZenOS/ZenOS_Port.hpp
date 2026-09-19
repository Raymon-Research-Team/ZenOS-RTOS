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
/* Compiler attributes — only define if not already provided by ZenOS_Compiler.hpp */
#ifndef OS_NAKED
#define OS_NAKED    __attribute__((naked))
#endif
#ifndef OS_USED
#define OS_USED     __attribute__((used))
#endif
#ifndef OS_ALIGNED
#define OS_ALIGNED(n) __attribute__((aligned(n)))
#endif
#ifndef OS_WEAK
#define OS_WEAK     __attribute__((weak))
#endif

/* ═══════════════ STM32 Family Detection ═══════════════
 * Each family maps to:
 *   OS_FAMILY_NAME      — human-readable string for banners
 *   OS_VECTOR_COUNT     — number of IRQs + 16 system exceptions
 *   OS_SMP_CORES        — number of cores (1 for all single-core STM32)
 *   OS_FLASH_SIZE       — typical flash size (bytes)
 *   OS_RAM_SIZE         — typical SRAM size (bytes)
 *   OS_CRC_BASE_ADDR    — CRC peripheral base address (varies by family)
 *   OS_IWDG_BASE_ADDR   — IWDG peripheral base address (varies by family)
 *   OS_RCC_CSR_ADDR     — RCC CSR register for reset flags
 *   OS_NVIC_PRIO_BITS   — number of implemented NVIC priority bits
 * ────────────────────────────────────────────────────────────
 * Core capability flags (FPU, MPU) are deliberately NOT set here: they are
 * derived further below from the CMSIS device header (__FPU_PRESENT /
 * __MPU_PRESENT) and the compiler's __ARM_ARCH_* macros, which are
 * authoritative.  A family name alone cannot prove that a silicon vendor
 * instantiated an optional component — STM32F303 is a Cortex-M4 without FPU.
 *
 * Vector counts are taken from the corresponding STM32 reference manuals.
 * For families with wide ranges (F2/F4/F7/H7), we use a mid-range value.
 * The actual VTOR size only matters when we copy the vector table in
 * os_start(); using the family's maximum is always safe. */

/* ── Cortex-M0 / M0+ (ARMv6-M) ──────────────────────
 * No BASEPRI, no MPU (M0) or optional MPU (M0+), Thumb-1 only.
 * These cores use a simpler exception model.
 *
 * ── Family-macro forms accepted below ────────────────────────────────────
 * A project built by STM32CubeMX defines the *part* macro (STM32F103xB,
 * STM32L072xx, STM32F030x6, ...) and NOT the STM32F1xx / STM32F1 family
 * macro; the latter is only defined once the CMSIS device header is
 * included, which ZenOS does further down.  ZenOS.hpp might be included
 * before main.h, so matching only STM32F1xx silently fell through to the
 * "Generic Cortex-M" defaults — wrong OS_FLASH_SIZE (128 KB instead of
 * 64 KB on F103C8) and, with this file's peripheral detection, an
 * undetected CRC/IWDG.
 *
 * OS_FAMILY_IS_STM32F1 is therefore defined here from the part macros that
 * the CMSIS device header itself checks (stm32f1xx.h line 59-61), so the
 * detection is order-independent and does not require the ST header to be
 * seen first. */
#if defined(STM32F103x6) || defined(STM32F103xB) || defined(STM32F103xE) || \
    defined(STM32F103xG) || defined(STM32F100xB) || defined(STM32F100xE) || \
    defined(STM32F101x6) || defined(STM32F101xB) || defined(STM32F101xE) || \
    defined(STM32F101xG) || defined(STM32F102x6) || defined(STM32F102xB) || \
    defined(STM32F105xC) || defined(STM32F107xC)
    #define OS_FAMILY_IS_STM32F1 1
#endif

#if defined(STM32F0xx) || defined(STM32F0) || defined(STM32F030x6) || \
    defined(STM32F030x8) || defined(STM32F031x6) || defined(STM32F042x6) || \
    defined(STM32F048xx) || defined(STM32F051x8) || defined(STM32F058xx) || \
    defined(STM32F070x6) || defined(STM32F070xB) || defined(STM32F072xB) || \
    defined(STM32F078xx) || defined(STM32F091xC) || defined(STM32F098xx)
    #define OS_FAMILY_IS_STM32F0 1
#endif

#if defined(STM32G0xx) || defined(STM32G0) || defined(STM32G030xx) || \
    defined(STM32G031xx) || defined(STM32G041xx) || defined(STM32G050xx) || \
    defined(STM32G051xx) || defined(STM32G061xx) || defined(STM32G070xx) || \
    defined(STM32G071xx) || defined(STM32G081xx) || defined(STM32G0B0xx) || \
    defined(STM32G0B1xx) || defined(STM32G0C1xx)
    #define OS_FAMILY_IS_STM32G0 1
#endif

#if defined(STM32L0xx) || defined(STM32L0) || defined(STM32L010x4) || \
    defined(STM32L010x6) || defined(STM32L010x8) || defined(STM32L010xB) || \
    defined(STM32L011xx) || defined(STM32L021xx) || defined(STM32L031xx) || \
    defined(STM32L041xx) || defined(STM32L051xx) || defined(STM32L052xx) || \
    defined(STM32L053xx) || defined(STM32L062xx) || defined(STM32L063xx) || \
    defined(STM32L071xx) || defined(STM32L072xx) || defined(STM32L073xx) || \
    defined(STM32L081xx) || defined(STM32L082xx) || defined(STM32L083xx)
    #define OS_FAMILY_IS_STM32L0 1
#endif

#if defined(STM32L1xx) || defined(STM32L1) || defined(STM32L100xB) || \
    defined(STM32L100xBA) || defined(STM32L100xC) || defined(STM32L151xB) || \
    defined(STM32L151xBA) || defined(STM32L151xC) || defined(STM32L151xCA) || \
    defined(STM32L151xD) || defined(STM32L151xDX) || defined(STM32L151xE) || \
    defined(STM32L152xB) || defined(STM32L152xBA) || defined(STM32L152xC) || \
    defined(STM32L152xCA) || defined(STM32L152xD) || defined(STM32L152xDX) || \
    defined(STM32L152xE) || defined(STM32L162xC) || defined(STM32L162xCA) || \
    defined(STM32L162xD) || defined(STM32L162xDX) || defined(STM32L162xE)
    #define OS_FAMILY_IS_STM32L1 1
#endif

#if defined(STM32F2xx) || defined(STM32F2) || defined(STM32F205xx) || \
    defined(STM32F207xx) || defined(STM32F215xx) || defined(STM32F217xx)
    #define OS_FAMILY_IS_STM32F2 1
#endif

#if defined(STM32F3xx) || defined(STM32F3) || defined(STM32F301x8) || \
    defined(STM32F302x8) || defined(STM32F302xC) || defined(STM32F302xE) || \
    defined(STM32F303x8) || defined(STM32F303xC) || defined(STM32F303xE) || \
    defined(STM32F318xx) || defined(STM32F328xx) || defined(STM32F334x8) || \
    defined(STM32F358xx) || defined(STM32F373xC) || defined(STM32F378xx) || \
    defined(STM32F398xx)
    #define OS_FAMILY_IS_STM32F3 1
#endif

#if defined(STM32F4xx) || defined(STM32F4) || defined(STM32F401xC) || \
    defined(STM32F401xE) || defined(STM32F405xx) || defined(STM32F407xx) || \
    defined(STM32F410Cx) || defined(STM32F410Rx) || defined(STM32F410Tx) || \
    defined(STM32F411xE) || defined(STM32F412Cx) || defined(STM32F412Rx) || \
    defined(STM32F412Vx) || defined(STM32F412Zx) || defined(STM32F413xx) || \
    defined(STM32F415xx) || defined(STM32F417xx) || defined(STM32F423xx) || \
    defined(STM32F427xx) || defined(STM32F429xx) || defined(STM32F437xx) || \
    defined(STM32F439xx) || defined(STM32F446xx) || defined(STM32F469xx) || \
    defined(STM32F479xx)
    #define OS_FAMILY_IS_STM32F4 1
#endif

#if defined(STM32F7xx) || defined(STM32F7) || defined(STM32F722xx) || \
    defined(STM32F723xx) || defined(STM32F732xx) || defined(STM32F733xx) || \
    defined(STM32F745xx) || defined(STM32F746xx) || defined(STM32F750xx) || \
    defined(STM32F756xx) || defined(STM32F765xx) || defined(STM32F767xx) || \
    defined(STM32F769xx) || defined(STM32F777xx) || defined(STM32F779xx)
    #define OS_FAMILY_IS_STM32F7 1
#endif

#if defined(STM32G4xx) || defined(STM32G4) || defined(STM32G431xx) || \
    defined(STM32G441xx) || defined(STM32G471xx) || defined(STM32G473xx) || \
    defined(STM32G474xx) || defined(STM32G483xx) || defined(STM32G484xx) || \
    defined(STM32GBK1CB)
    #define OS_FAMILY_IS_STM32G4 1
#endif

#if defined(STM32H7xx) || defined(STM32H7) || defined(STM32H723xx) || \
    defined(STM32H725xx) || defined(STM32H730xx) || defined(STM32H735xx) || \
    defined(STM32H742xx) || defined(STM32H743xx) || defined(STM32H745xx) || \
    defined(STM32H747xx) || defined(STM32H750xx) || defined(STM32H753xx) || \
    defined(STM32H755xx) || defined(STM32H757xx) || defined(STM32H7A3xx) || \
    defined(STM32H7B3xx)
    #define OS_FAMILY_IS_STM32H7 1
#endif

#if defined(STM32L4xx) || defined(STM32L4) || defined(STM32L412xx) || \
    defined(STM32L422xx) || defined(STM32L431xx) || defined(STM32L432xx) || \
    defined(STM32L433xx) || defined(STM32L442xx) || defined(STM32L443xx) || \
    defined(STM32L451xx) || defined(STM32L452xx) || defined(STM32L462xx) || \
    defined(STM32L471xx) || defined(STM32L475xx) || defined(STM32L476xx) || \
    defined(STM32L485xx) || defined(STM32L486xx) || defined(STM32L496xx) || \
    defined(STM32L4A6xx) || defined(STM32L4P5xx) || defined(STM32L4Q5xx) || \
    defined(STM32L4R5xx) || defined(STM32L4R7xx) || defined(STM32L4R9xx) || \
    defined(STM32L4S5xx) || defined(STM32L4S7xx) || defined(STM32L4S9xx)
    #define OS_FAMILY_IS_STM32L4 1
#endif

#if defined(STM32L5xx) || defined(STM32L5) || defined(STM32L552xx) || \
    defined(STM32L562xx)
    #define OS_FAMILY_IS_STM32L5 1
#endif

#if defined(STM32U5xx) || defined(STM32U5) || defined(STM32U535xx) || \
    defined(STM32U545xx) || defined(STM32U575xx) || defined(STM32U585xx) || \
    defined(STM32U595xx) || defined(STM32U599xx) || defined(STM32U5A5xx) || \
    defined(STM32U5A9xx)
    #define OS_FAMILY_IS_STM32U5 1
#endif

#if defined(STM32WBxx) || defined(STM32WB) || defined(STM32WB10xx) || \
    defined(STM32WB15xx) || defined(STM32WB1Mxx) || defined(STM32WB35xx) || \
    defined(STM32WB50xx) || defined(STM32WB55xx) || defined(STM32WB5Mxx)
    #define OS_FAMILY_IS_STM32WB 1
#endif

#if defined(STM32WBAxx) || defined(STM32WBA) || defined(STM32WBA52xx) || \
    defined(STM32WBA54xx) || defined(STM32WBA55xx)
    #define OS_FAMILY_IS_STM32WBA 1
#endif

#if defined(OS_FAMILY_IS_STM32F0)
    #define OS_FAMILY_NAME      "STM32F0"
    #define OS_VECTOR_COUNT     32
    #define OS_SMP_CORES        1
    #define OS_FLASH_SIZE       0x10000UL    /* 64KB (F030) */
    #define OS_RAM_SIZE         0x2000UL     /* 8KB */
    #define OS_NVIC_PRIO_BITS   2
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40021024UL

#elif defined(OS_FAMILY_IS_STM32G0)
    #define OS_FAMILY_NAME      "STM32G0"
    #define OS_VECTOR_COUNT     32
    #define OS_SMP_CORES        1
    #define OS_FLASH_SIZE       0x20000UL    /* 128KB (G070) */
    #define OS_RAM_SIZE         0x5000UL     /* 20KB */
    #define OS_NVIC_PRIO_BITS   2
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40021094UL /* G0 RCC_CSR @ base 0x40021000 + 0x94 */

/* ── Cortex-M3 (ARMv7-M) ──────────────────────────────
 * No FPU, optional MPU (M3), Thumb-2, BASEPRI available. */
#elif defined(OS_FAMILY_IS_STM32F1)
    #define OS_FAMILY_NAME      "STM32F1"
    #define OS_VECTOR_COUNT     68
    #define OS_SMP_CORES        1
    #define OS_FLASH_SIZE       0x10000UL    /* 64KB (F103C8) */
    #define OS_RAM_SIZE         0x5000UL     /* 20KB */
    #define OS_NVIC_PRIO_BITS   4
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40021024UL

#elif defined(OS_FAMILY_IS_STM32L0)
    #define OS_FAMILY_NAME      "STM32L0"
    #define OS_VECTOR_COUNT     32
    #define OS_SMP_CORES        1
    #define OS_FLASH_SIZE       0x20000UL    /* 128KB (L072) */
    #define OS_RAM_SIZE         0x5000UL     /* 20KB */
    #define OS_NVIC_PRIO_BITS   2
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40021024UL

#elif defined(OS_FAMILY_IS_STM32L1)
    #define OS_FAMILY_NAME      "STM32L1"
    #define OS_VECTOR_COUNT     68
    #define OS_SMP_CORES        1
    #define OS_FLASH_SIZE       0x100000UL   /* 1MB (L152) */
    #define OS_RAM_SIZE         0x14000UL    /* 80KB */
    #define OS_NVIC_PRIO_BITS   4
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40023824UL

/* ── Cortex-M3 / Cortex-M4 (no FPU) ────────────────── */
#elif defined(OS_FAMILY_IS_STM32F2)
    #define OS_FAMILY_NAME      "STM32F2"
    #define OS_VECTOR_COUNT     81
    #define OS_SMP_CORES        1
    #define OS_FLASH_SIZE       0x100000UL   /* 1MB (F207) */
    #define OS_RAM_SIZE         0x20000UL    /* 128KB */
    #define OS_NVIC_PRIO_BITS   4
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40023824UL

#elif defined(OS_FAMILY_IS_STM32F3)
    #define OS_FAMILY_NAME      "STM32F3"
    #define OS_VECTOR_COUNT     82
    #define OS_SMP_CORES        1
    #define OS_FLASH_SIZE       0x40000UL    /* 256KB (F303) */
    #define OS_RAM_SIZE         0x10000UL    /* 64KB */
    #define OS_NVIC_PRIO_BITS   4
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40021024UL

/* ── Cortex-M4F (ARMv7E-M with FPU) ────────────────── */
#elif defined(OS_FAMILY_IS_STM32F4)
    #define OS_FAMILY_NAME      "STM32F4"
    #define OS_VECTOR_COUNT     86
    #define OS_SMP_CORES        1
    #define OS_FLASH_SIZE       0x200000UL   /* 2MB (F429) */
    #define OS_RAM_SIZE         0x20000UL    /* 128KB */
    #define OS_NVIC_PRIO_BITS   4
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40023824UL

#elif defined(OS_FAMILY_IS_STM32G4)
    #define OS_FAMILY_NAME      "STM32G4"
    #define OS_VECTOR_COUNT     100
    #define OS_SMP_CORES        1
    #define OS_FLASH_SIZE       0x80000UL    /* 512KB (G474) */
    #define OS_RAM_SIZE         0x18000UL    /* 96KB */
    #define OS_NVIC_PRIO_BITS   4
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40021024UL

/* ── Cortex-M7 (ARMv7E-M with FPU) ─────────────────── */
#elif defined(OS_FAMILY_IS_STM32F7)
    #define OS_FAMILY_NAME      "STM32F7"
    #define OS_VECTOR_COUNT     91
    #define OS_SMP_CORES        1
    #define OS_FLASH_SIZE       0x200000UL   /* 2MB (F767) */
    #define OS_RAM_SIZE         0x40000UL    /* 256KB */
    #define OS_NVIC_PRIO_BITS   4
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40023824UL

#elif defined(OS_FAMILY_IS_STM32H7)
    #define OS_FAMILY_NAME      "STM32H7"
    #define OS_VECTOR_COUNT     150
    #define OS_SMP_CORES        1
    #define OS_FLASH_SIZE       0x200000UL   /* 2MB (H743) */
    #define OS_RAM_SIZE         0x40000UL    /* 256KB typical (H743 = 1MB across DTCM+AXI+SRAM1-4) */
    #define OS_NVIC_PRIO_BITS   4
    #define OS_CRC_BASE_ADDR    0x58024C00UL /* H7 uses AHB4 bus */
    #define OS_IWDG_BASE_ADDR   0x58004800UL /* H7 uses APB4 bus */
    #define OS_RCC_CSR_ADDR     0x580244D0UL /* H7 uses RCC->CSR */

/* ── Cortex-M4F Low-Power ──────────────────────────── */
#elif defined(OS_FAMILY_IS_STM32L4)
    #define OS_FAMILY_NAME      "STM32L4"
    #define OS_VECTOR_COUNT     82
    #define OS_SMP_CORES        1
    #define OS_FLASH_SIZE       0x100000UL   /* 1MB (L496) */
    #define OS_RAM_SIZE         0x20000UL    /* 128KB */
    #define OS_NVIC_PRIO_BITS   4
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40021024UL

#elif defined(OS_FAMILY_IS_STM32L5)
    #define OS_FAMILY_NAME      "STM32L5"
    #define OS_VECTOR_COUNT     82
    #define OS_SMP_CORES        1
    #define OS_FLASH_SIZE       0x100000UL   /* 512KB */
    #define OS_RAM_SIZE         0x20000UL    /* 256KB */
    #define OS_NVIC_PRIO_BITS   4
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40021024UL

#elif defined(OS_FAMILY_IS_STM32U5)
    #define OS_FAMILY_NAME      "STM32U5"
    #define OS_VECTOR_COUNT     82
    #define OS_SMP_CORES        1
    #define OS_FLASH_SIZE       0x200000UL   /* 2MB (U585) */
    #define OS_RAM_SIZE         0x20000UL    /* 256KB */
    #define OS_NVIC_PRIO_BITS   4
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40021024UL

#elif defined(OS_FAMILY_IS_STM32WB)
    #define OS_FAMILY_NAME      "STM32WB"
    #define OS_VECTOR_COUNT     66
    #define OS_SMP_CORES        1
    #define OS_FLASH_SIZE       0x100000UL   /* 1MB (WB55) */
    #define OS_RAM_SIZE         0x40000UL    /* 256KB */
    #define OS_NVIC_PRIO_BITS   4
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40021024UL

#elif defined(OS_FAMILY_IS_STM32WBA)
    #define OS_FAMILY_NAME      "STM32WBA"
    #define OS_VECTOR_COUNT     82
    #define OS_SMP_CORES        1
    #define OS_FLASH_SIZE       0x200000UL   /* 1MB (WBA55) */
    #define OS_RAM_SIZE         0x40000UL    /* 256KB */
    #define OS_NVIC_PRIO_BITS   4
    #define OS_CRC_BASE_ADDR    0x40023000UL
    #define OS_IWDG_BASE_ADDR   0x40003000UL
    #define OS_RCC_CSR_ADDR     0x40021024UL

/* ── Fallback: unknown STM32 or generic Cortex-M ────
 * Only the peripheral-memory map is defaulted here; FPU/MPU capability is
 * resolved afterwards from CMSIS/__ARM_ARCH_* for every target, including
 * this one. */
#else
    #define OS_FAMILY_NAME      "Generic Cortex-M"
    #define OS_VECTOR_COUNT     68
    #define OS_SMP_CORES        1
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
/* RAM test start: derived from linker symbol __bss_end__ so the test
   always begins AFTER all static variables, avoiding corruption of
   live OS data (.data, .bss, stack). */
extern uint32_t __bss_end__;
#define OS_RAM_TEST_START    ((uintptr_t)&__bss_end__)
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

/* Runtime guard: OS_RAM_TEST_START must fall below OS_RAM_TEST_END.
   Checked in os_ram_test_step() on first call. */

/* ═══════════════ CMSIS Device Header (capability source of truth) ═══════
 * The device header defines __MPU_PRESENT and __FPU_PRESENT, which describe
 * what the *silicon* actually contains.  Those two macros are authoritative
 * and must be in scope before the capability block below runs, so we pull
 * the header in here rather than relying on the application having included
 * it first.
 *
 * The header is selected from the same STM32 family macro used above, so an
 * application that only includes ZenOS.hpp still gets correct capability
 * detection.  If the header cannot be found (a non-STM32 Cortex-M project,
 * or a build that does not put the CMSIS device directory on the include
 * path) we fall back to __ARM_ARCH_*, and the capability block reports the
 * weaker evidence through #warning where it matters.
 *
 * HAL-INDEPENDENCE: when the project defines USE_HAL_DRIVER (as every
 * STM32CubeMX project does), the ST device header itself pulls in the HAL
 * ("stm32f1xx_hal.h").  The kernel does not want that dependency, so we
 * temporarily undefine USE_HAL_DRIVER around the include and restore it
 * afterwards.  The application still sees its original macro; only the
 * kernel's view of the device header is kept HAL-free. */
#ifndef OS_NO_CMSIS_DEVICE_HEADER
    #ifdef USE_HAL_DRIVER
        #define OS_USE_HAL_DRIVER_SAVED 1
        #undef USE_HAL_DRIVER
    #endif
    #if defined(OS_FAMILY_IS_STM32F0)
        #include "stm32f0xx.h"
    #elif defined(OS_FAMILY_IS_STM32F1)
        #include "stm32f1xx.h"
    #elif defined(OS_FAMILY_IS_STM32F2)
        #include "stm32f2xx.h"
    #elif defined(OS_FAMILY_IS_STM32F3)
        #include "stm32f3xx.h"
    #elif defined(OS_FAMILY_IS_STM32F4)
        #include "stm32f4xx.h"
    #elif defined(OS_FAMILY_IS_STM32F7)
        #include "stm32f7xx.h"
    #elif defined(OS_FAMILY_IS_STM32G0)
        #include "stm32g0xx.h"
    #elif defined(OS_FAMILY_IS_STM32G4)
        #include "stm32g4xx.h"
    #elif defined(OS_FAMILY_IS_STM32H7)
        #include "stm32h7xx.h"
    #elif defined(OS_FAMILY_IS_STM32L0)
        #include "stm32l0xx.h"
    #elif defined(OS_FAMILY_IS_STM32L1)
        #include "stm32l1xx.h"
    #elif defined(OS_FAMILY_IS_STM32L4)
        #include "stm32l4xx.h"
    #elif defined(OS_FAMILY_IS_STM32L5)
        #include "stm32l5xx.h"
    #elif defined(OS_FAMILY_IS_STM32U5)
        #include "stm32u5xx.h"
    #elif defined(OS_FAMILY_IS_STM32WB)
        #include "stm32wbxx.h"
    #elif defined(OS_FAMILY_IS_STM32WBA)
        #include "stm32wbaxx.h"
    #endif
    #ifdef OS_USE_HAL_DRIVER_SAVED
        #undef OS_USE_HAL_DRIVER_SAVED
        #define USE_HAL_DRIVER
    #endif
#endif /* OS_NO_CMSIS_DEVICE_HEADER */

/* ═══════════════ ARM Architecture Detection ═══════════════
 * The compiler defines exactly one of these from -mcpu (authoritative):
 *   __ARM_ARCH_6M__   — Cortex-M0, M0+, M1 (ARMv6-M)
 *   __ARM_ARCH_7M__   — Cortex-M3 (ARMv7-M)
 *   __ARM_ARCH_7EM__  — Cortex-M4, M7 (ARMv7E-M)
 *   __ARM_ARCH_8M_BASE__ — Cortex-M23 (ARMv8-M Baseline)
 *   __ARM_ARCH_8M_MAIN__ — Cortex-M33 (ARMv8-M Mainline)
 */

/* ═══════════════ MPU Region Count (PMSAv7 / PMSAv8) ═══════════════
 * Region-count is a property of the MPU *implementation*, not of the core
 * revision, so it must be read from the device's CMSIS header when that
 * header is available (__MPU_PRESENT == 1 implies <core>_MPU_TYPE / the
 * CMSIS `MPU->TYPE` DREGION field).  Only when no CMSIS device header is
 * in the include path do we fall back to the architecture maximum, which is
 * a safe upper bound because the driver checks the live DREGION at runtime
 * (see os_mpu_init). */
#if defined(__ARM_ARCH_8M_MAIN__) || defined(__ARM_ARCH_8M_BASE__)
    #define OS_MPU_MAX_REGIONS  16   /* PMSAv8: 0..15 architectural maximum */
#else
    #define OS_MPU_MAX_REGIONS  8    /* PMSAv7: 0..7 architectural maximum   */
#endif

/* ═══════════════ MPU Availability ═══════════════
 * ARMv7-M (Cortex-M3/M4/M7) and ARMv8-M (Cortex-M23/M33) make the MPU an
 * OPTIONAL component that the silicon vendor may or may not instantiate;
 * ARMv6-M (Cortex-M0/M0+) has no PMSA MPU at all (the M0+ MPU is an
 * implementation-defined extension that ST does not offer on any STM32).
 *
 * The authoritative source is the device's CMSIS header, which defines
 * __MPU_PRESENT.  Using the core name alone is NOT sufficient — e.g.
 * Cortex-M3 implies PMSAv7 hardware but STM32F1's device header still has
 * to confirm it.  We therefore:
 *   1. use __MPU_PRESENT when a CMSIS device header was included, and
 *   2. otherwise derive from the architecture, with ARMv6-M always 0.
 */
#if defined(__MPU_PRESENT)
    #define OS_HAS_MPU_HW   (__MPU_PRESENT ? 1 : 0)
#elif defined(__ARM_ARCH_6M__)
    #define OS_HAS_MPU_HW   0
#else
    #define OS_HAS_MPU_HW   1
#endif

/* Reject the configuration at compile time when the user asks for MPU
 * protection on a core that has no MPU.  Silently compiling the PMSAv7
 * path for Cortex-M0 would emit stores to 0xE000ED90 (which does not
 * exist on ARMv6-M) and fault at the first context switch. */
#if OS_SAFETY_MPU_EN && !OS_HAS_MPU_HW
    #error "[ZenOS] OS_SAFETY_MPU_EN=1 but this target has no MPU (Cortex-M0/M0+ or device header reports __MPU_PRESENT=0). Set OS_SAFETY_MPU_EN=0 or select a Cortex-M3/M4/M7/M23/M33 device."
#endif

/* Report an unverified MPU configuration.  When no CMSIS device header was
 * included we cannot prove the MPU exists; the user must confirm it. */
#if OS_SAFETY_MPU_EN && !defined(__MPU_PRESENT)
    #warning "[ZenOS] OS_SAFETY_MPU_EN=1 without a CMSIS device header in the include path; MPU presence was assumed from __ARM_ARCH_* only. Include the device header or confirm __MPU_PRESENT for your part."
#endif

/* Whether user tasks actually run with an active MPU (the safety feature is
   on AND the silicon has one).  Reported through os_get_capabilities(). */
#if OS_SAFETY_MPU_EN && OS_HAS_MPU_HW
    #define OS_HAS_USER_MPU_ACTIVE 1
#else
    #define OS_HAS_USER_MPU_ACTIVE 0
#endif

/* ═══════════════ Peripheral Availability (CMSIS only) ═══════════════
 * The CRC and IWDG blocks are absent on some STM32 parts (e.g. STM32F0/L0
 * value lines).  Touching their registers on such a device faults, so the
 * availability must be proven before the safety code is compiled in.
 *
 * The authoritative signal is the CMSIS *device* header, which either
 * defines the peripheral base (CRC_BASE / IWDG_BASE) or omits it entirely.
 * ZenOS deliberately does NOT use HAL_IWDG_MODULE_ENABLED /
 * HAL_CRC_MODULE_ENABLED for this: those are HAL build-configuration
 * switches, not statements about the silicon, and depending on them would
 * couple the kernel API to the HAL.
 *
 * A project may override either flag explicitly with
 *   -DOS_HAS_CRC_HW=0 / -DOS_HAS_IWDG_HW=1
 * when building without the CMSIS device header (e.g. a bare-metal port),
 * in which case the override is trusted. */
#ifndef OS_HAS_CRC_HW
    #if defined(CRC_BASE) || defined(CRC)
        #define OS_HAS_CRC_HW  1
    #else
        #define OS_HAS_CRC_HW  0
    #endif
#endif

#ifndef OS_HAS_IWDG_HW
    #if defined(IWDG_BASE) || defined(IWDG)
        #define OS_HAS_IWDG_HW 1
    #else
        #define OS_HAS_IWDG_HW 0
    #endif
#endif

/* A safety feature that needs a peripheral the device does not have cannot
 * be satisfied — fail loudly instead of emitting dead register accesses. */
#if OS_SAFETY_CRC_EN && !OS_HAS_CRC_HW
    #error "[ZenOS] OS_SAFETY_CRC_EN=1 but this device has no CRC peripheral (CRC_BASE is not defined by its CMSIS header). Set OS_SAFETY_CRC_EN=0 or select a part with a CRC unit."
#endif

#if OS_SAFETY_HW_WATCHDOG_EN && !OS_HAS_IWDG_HW
#error "[ZenOS] OS_SAFETY_HW_WATCHDOG_EN=1 but this device has no IWDG peripheral (IWDG_BASE is not defined by its CMSIS header). Set OS_SAFETY_HW_WATCHDOG_EN=0 or select a part with an IWDG."
#endif

/* ═══════════════ Capability Bitmask (os_get_capabilities) ═══════════════
 * Single numeric summary of everything detected at compile time, so an
 * application can assert on the target instead of re-deriving it. */
#define OS_CAP_MPU         (1UL << 0)   /* MPU hardware present on the silicon */
#define OS_CAP_FPU         (1UL << 1)   /* FPU hardware present on the silicon */
#define OS_CAP_MPU_ACTIVE  (1UL << 2)   /* OS_SAFETY_MPU_EN=1 and MPU available  */
#define OS_CAP_CRC_HW      (1UL << 3)   /* CRC peripheral available+enabled  */
#define OS_CAP_IWDG_HW     (1UL << 4)   /* IWDG peripheral available+enabled */
#define OS_CAP_CYCCNT      (1UL << 5)   /* DWT CYCCNT counter available      */
#define OS_CAP_VTOR        (1UL << 6)   /* SCB->VTOR available               */
#define OS_CAP_TICKLESS    (1UL << 7)   /* OS_KERNEL_TICKLESS_IDLE_EN compiled in */

/* ═══════════════ FPU Availability ═══════════════
 * Same rule as the MPU: a Cortex-M4F/M7F only has the FPU when the vendor
 * instantiated it (STM32F303 is a Cortex-M4 *without* FPU).  The device
 * header's __FPU_PRESENT is authoritative; otherwise ARMv7E-M/ARMv8-M
 * Mainline is assumed to have one and ARMv6-M/ARMv7-M do not. */
#if defined(__FPU_PRESENT)
    #define OS_HAS_FPU_HW   (__FPU_PRESENT ? 1 : 0)
#elif defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_MAIN__)
    #define OS_HAS_FPU_HW   1
#else
    #define OS_HAS_FPU_HW   0
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
 * This is a porting concern — see PORTING_GUIDE.md.
 *
 * Only Cortex-M7 (and the M7-based STM32F7/H7 parts) has the data cache, so
 * the flag defaults to 0 there and to 1 elsewhere is never needed.  A user
 * targeting F7/H7 with the cache enabled MUST define OS_HAS_CACHE=1, so a
 * configuration that leaves it at 0 on a cache-capable part is reported
 * instead of silently running without the required cache maintenance. */
#ifndef OS_HAS_CACHE
    #if defined(STM32F7xx) || defined(STM32H7xx)
        #define OS_HAS_CACHE  0
    #else
        #define OS_HAS_CACHE  0
    #endif
#endif

#if OS_HAS_CACHE
    #if !defined(STM32F7xx) && !defined(STM32H7xx) && !defined(__ARM_ARCH_7EM__)
        #error "[ZenOS] OS_HAS_CACHE=1 but the selected target has no Cortex-M7 data cache. Clear OS_HAS_CACHE or select an STM32F7/H7 device."
    #endif
    #warning "[ZenOS] OS_HAS_CACHE=1: kernel TCB/stack memory is assumed non-cacheable (MPU-backed) or explicitly cleaned. Verify the MPU/cache configuration in PORTING_GUIDE.md."
#endif

/* ═══════════════ Peripheral Consistency — Application Layer ═══════════════
 * The complementary check ("CubeMX enables IWDG/CRC but the matching ZenOS
 * safety feature is off, so the peripheral runs unsupervised") needs to know
 * the *project's* HAL configuration and therefore belongs to the application,
 * not to the kernel.  ZenOS keeps no HAL/LL dependency: this header states
 * only what the silicon provides (OS_HAS_IWDG_HW / OS_HAS_CRC_HW above).
 *
 * See Core/Src/main.cpp in the sample project for a working example of that
 * application-side check.
 */
