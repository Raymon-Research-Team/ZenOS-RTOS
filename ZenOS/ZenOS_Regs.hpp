#pragma once
/**
 * @file    ZenOS_Regs.hpp
 * @brief   ZenOS RTOS — Consolidated Cortex-M Register Definitions
 *
 * ALL raw register addresses for SCB, NVIC, SysTick, DWT, MPU, IWDG, CRC
 * are defined here as typed structs or macros. NO other file should define
 * these addresses directly — they MUST include this header instead.
 *
 * @author  Rahman Heidari <rahman.h22@gmail.com> — Raymon Research Team
 * @version 1.1.0
 */

#include <stdint.h>

/* ═══════════════ SCB (System Control Block) ═══════════════
 * Base: 0xE000ED00 (ARMv7-M / ARMv8-M)
 * Reference: ARM Cortex-M3/M4/M7/M23/M33 Technical Reference Manual
 */
#define OS_SCB_BASE        0xE000ED00UL

typedef struct {
    volatile uint32_t CPUID;    /* 0x00 RO  CPUID Base Register              */
    volatile uint32_t ICSR;     /* 0x04 RW  Interrupt Control and State       */
    volatile uint32_t VTOR;     /* 0x08 RW  Vector Table Offset              */
    volatile uint32_t AIRCR;    /* 0x0C RW  Application Interrupt/Reset Ctrl  */
    volatile uint32_t SCR;      /* 0x10 RW  System Control                   */
    volatile uint32_t CCR;      /* 0x14 RW  Configuration and Control         */
    volatile uint32_t SHPR[3];  /* 0x18-0x1F RW  System Handler Priority      */
    volatile uint32_t SHCSR;    /* 0x20 RW  System Handler Control and State  */
    volatile uint32_t CFSR;     /* 0x24 RW  Configurable Fault Status         */
    volatile uint32_t HFSR;     /* 0x28 RW  HardFault Status                  */
    volatile uint32_t DFSR;     /* 0x2C RW  Debug Fault Status                */
    volatile uint32_t MMFAR;    /* 0x30 RW  MemManage Fault Address           */
    volatile uint32_t BFAR;     /* 0x34 RW  BusFault Address                  */
    volatile uint32_t AFSR;     /* 0x38 RW  Auxiliary Fault Status            */
} OS_SCB_Type;

#define OS_SCB ((OS_SCB_Type*)OS_SCB_BASE)

/* Convenience aliases (backward-compatible with existing code) */
#define OS_SCB_ICSR   OS_SCB->ICSR
#define OS_SCB_VTOR   OS_SCB->VTOR

/* ICSR bit masks */
#define OS_ICSR_PENDSVSET_Msk (1UL << 28)

/* ═══════════════ SysTick Timer ═══════════════
 * Base: 0xE000E010 (ARMv7-M / ARMv8-M)
 */
#define OS_SYST_BASE       0xE000E010UL

typedef struct {
    volatile uint32_t CSR;   /* 0x00 RW  Control and Status                  */
    volatile uint32_t RVR;   /* 0x04 RW  Reload Value                       */
    volatile uint32_t CVR;   /* 0x08 RW  Current Value                      */
    volatile uint32_t CALIB; /* 0x0C RO  Calibration                        */
} OS_SYST_Type;

#define OS_SYST ((OS_SYST_Type*)OS_SYST_BASE)

/* Convenience aliases (backward-compatible with existing code) */
#define OS_SYST_CSR   OS_SYST->CSR
#define OS_SYST_RVR   OS_SYST->RVR
#define OS_SYST_CVR   OS_SYST->CVR

/* CSR bit masks */
#define OS_SYST_CSR_ENABLE_Msk    (1UL << 0)
#define OS_SYST_CSR_TICKINT_Msk   (1UL << 1)
#define OS_SYST_CSR_COUNTFLAG_Msk (1UL << 16)

/* ═══════════════ NVIC Priority Registers ═══════════════
 * SHPR2 (SVCall) @ 0xE000ED1C
 * SHPR3 (PendSV + SysTick) @ 0xE000ED20
 * Written as byte-accessible for PendSV and SysTick priorities.
 */
#define OS_PENDSV_PRIO    (*((volatile uint8_t*)0xE000ED22UL))
#define OS_SYSTICK_PRIO   (*((volatile uint8_t*)0xE000ED23UL))

/* ═══════════════ DWT (Data Watchpoint and Trace) ═══════════════
 * Base: 0xE0001000 (present on Cortex-M3+, absent on M0/M0+)
 */
#define OS_DWT_BASE        0xE0001000UL

typedef struct {
    volatile uint32_t CTRL;    /* 0x00 RW  Control                           */
    volatile uint32_t CYCCNT;  /* 0x04 RW  Cycle Count                      */
    volatile uint32_t CPICNT;  /* 0x08 RO  CPI Count                        */
    volatile uint32_t EXCCNT;  /* 0x0C RO  Exception Overhead Count         */
    volatile uint32_t SLEEPCNT;/* 0x10 RO  Sleep Count                       */
    volatile uint32_t LSUCNT;  /* 0x14 RO  LSU Count                         */
    volatile uint32_t FOLDCNT; /* 0x18 RO  Folded-instruction Count          */
    volatile uint32_t PCSR;    /* 0x1C RO  Program Counter Sample            */
} OS_DWT_Type;

#define OS_DWT ((OS_DWT_Type*)OS_DWT_BASE)

/* Convenience aliases (backward-compatible with existing code) */
#define OS_DWT_CTRL   OS_DWT->CTRL
#define OS_DWT_CYCCNT OS_DWT->CYCCNT

/* CTRL bit masks */
#define OS_DWT_CTRL_CYCCNTENA_Msk (1UL << 0)

/* CoreDebug DEMCR for trace enable */
#define OS_COREDEBUG_DEMCR (*(volatile uint32_t*)0xE000EDFCUL)
#define OS_COREDEM_TRCENA  (1UL << 24)
#define OS_DWT_CYCCNTENA   (1UL << 0)

/* ═══════════════ MPU (Memory Protection Unit) ═══════════════
 * Base: 0xE000ED90 (present on Cortex-M3/M4/M7/M23/M33)
 * Layout differs between PMSAv7 and PMSAv8 — see OS_MPU_Type below.
 */

/* PMSAv8 MPU register layout (ARMv8-M, Cortex-M23/M33) */
#if defined(__ARM_ARCH_8M_MAIN__) || defined(__ARM_ARCH_8M_BASE__)
    #define OS_MPU_PMSAv8  1
    #define OS_MPU_PMSAv7  0
    typedef struct {
        volatile uint32_t TYPE;   /* 0x00 RO  MPU Type Register        */
        volatile uint32_t CTRL;   /* 0x04 RW  MPU Control Register     */
        volatile uint32_t RNR;    /* 0x08 RW  MPU Region Number        */
        volatile uint32_t RBAR;   /* 0x0C RW  MPU Region Base Address  */
        volatile uint32_t RLAR;   /* 0x10 RW  MPU Region Limit Address */
    } OS_MPU_Type;
#else
    #define OS_MPU_PMSAv8  0
    #define OS_MPU_PMSAv7  1
    typedef struct {
        volatile uint32_t TYPE;   /* 0x00 RO  MPU Type Register                    */
        volatile uint32_t CTRL;   /* 0x04 RW  MPU Control Register                 */
        volatile uint32_t RNR;    /* 0x08 RW  MPU Region Number Register           */
        volatile uint32_t RBAR;   /* 0x0C RW  MPU Region Base Address Register     */
        volatile uint32_t RASR;   /* 0x10 RW  MPU Region Attribute & Size Register */
    } OS_MPU_Type;
#endif

#define OS_MPU_BASE ((OS_MPU_Type*)0xE000ED90UL)

/* ═══════════════ IWDG (Independent Watchdog) ═══════════════
 * Base addresses vary by STM32 family — defined in ZenOS_Port.hpp.
 * This file provides the register offsets relative to the base.
 */
#ifndef OS_IWDG_BASE_ADDR
    #define OS_IWDG_BASE_ADDR  0x40003000UL
#endif
#define OS_IWDG_KR  (*((volatile uint32_t*)(OS_IWDG_BASE_ADDR + 0x00UL)))
#define OS_IWDG_SR  (*((volatile uint32_t*)(OS_IWDG_BASE_ADDR + 0x0CUL)))

/* ═══════════════ CRC (Cyclic Redundancy Check) ═══════════════
 * Base addresses vary by STM32 family — defined in ZenOS_Port.hpp.
 * This file provides the register offsets relative to the base.
 */
#ifndef OS_CRC_BASE_ADDR
    #define OS_CRC_BASE_ADDR  0x40023000UL
#endif
#define OS_CRC_DR  (*((volatile uint32_t*)(OS_CRC_BASE_ADDR + 0x00UL)))
#define OS_CRC_CR  (*((volatile uint32_t*)(OS_CRC_BASE_ADDR + 0x08UL)))
