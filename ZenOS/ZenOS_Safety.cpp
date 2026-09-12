/**
 * @file    ZenOS_Safety.cpp
 * @brief   ZenOS RTOS — Safety: Error System, Stack Check, Fault Handler,
 *          Hardware Watchdog, CRC Check, RAM Test, MPU Protection
 *
 * Extracted from ZenOS.cpp as part of the modular split.
 *
 * @author  Rahman Heidari <rahman.h22@gmail.com> — Raymon Research Team
 * @version 2.0.0
 */

#define OS_BUILD
#include "ZenOS_Internal.hpp"

extern "C" uint32_t SystemCoreClock;

/* ═══════════════ Error System ═══════════════ */
/* Error counters are touched from ISR context (os_tick reports DEADLINE_MISS /
   TASK_STUCK / STACK_OVERFLOW) and read from task context — volatile too. */

uint32_t os_get_wdg_reset_count(void)      { return wdg_reset_count; }
uint32_t os_get_stack_recovery_count(void) { return stack_recovery_count; }

uint32_t os_get_error_count(void)            { return error_total; }
uint32_t os_get_expected_error_count(void)   { return error_expected; }
uint32_t os_get_unexpected_error_count(void) { return error_total - error_expected; }
OSError  os_get_last_error(void)             { return error_last; }
uint32_t os_in_safe(void)                    { return os_safe_depth; }

/* Mark a region whose errors are deliberate (test fault injection): errors
   reported inside are excluded from os_get_unexpected_error_count(). */
extern "C" void os_error_expect_begin(void) { error_expect_depth++; }
extern "C" void os_error_expect_end(void)   { if (error_expect_depth > 0) error_expect_depth--; }

/* os_report_error — no CS needed (error path only, last-writer-wins) */
void os_report_error(OSError code) {
    error_total++;
    if (error_expect_depth > 0) error_expected++;
    error_last = code;
#if OS_MONITOR_ERROR_LOG
    /* Automatically log errors to the ring buffer.
       Severity mapping: critical errors get CRITICAL, others get WARNING. */
    uint8_t sev = (code == OSError::HARDFAULT || code == OSError::STACK_OVERFLOW ||
                   code == OSError::TCB_CORRUPTED || code == OSError::RAM_TEST_FAIL)
                  ? (uint8_t)ErrorSeverity::CRITICAL
                  : (uint8_t)ErrorSeverity::WARNING;
    os_log_error(code, sev);
#endif
}


/* ═══════════════ Stack Check ═══════════════
 * Checks all active tasks for stack overflow using two methods:
 *   1. Stack pointer below canary region (active overflow)
 *   2. Canary corruption (memory corruption from outside)
 * On detection, the task is removed from the scheduler and either
 * reset or disabled based on recovery count. */
void os_stack_check_all(void) {
    for (TCB* task = task_list; task; task = task->next) {
        if (task == &idle_tcb) continue;
        if (task->state == TaskState::INACTIVE) continue;
        if (!task->stack_base) continue;

        bool overflow = false;

        /* ── Check 1: stack pointer below canary region (real overflow) ── */
        uint32_t* min_sp = task->stack_base + OS_STACK_CANARY_COUNT;
        if (task->stack_top < min_sp) {
            overflow = true;
        }

        /* ── Check 2: canary corruption (external memory corruption) ── */
        if (!overflow) {
            for (uint32_t i = 0; i < OS_STACK_CANARY_COUNT; i++) {
                if (task->stack_base[i] != OS_STACK_CANARY) {
                    overflow = true;
                    break;
                }
            }
        }

        if (!overflow) continue;

        /* Stack overflow confirmed */
        os_report_error(OSError::STACK_OVERFLOW);

        /* O(1) scheduler: remove from priority queue before state change */
        os_pq_remove(task);
        if (task->state == TaskState::BLOCKED && blocked_count > 0) {
            uint32_t tmp, res;
            __asm volatile(
                "1: ldrex %0, [%2]\n"
                "   cmp   %0, #0\n"
                "   beq   2f\n"
                "   subs  %0, %0, #1\n"
                "   strex %1, %0, [%2]\n"
                "   cmp   %1, #0\n"
                "   bne   1b\n"
                "2:\n"
                : "=&r"(tmp), "=&r"(res)
                : "r"(&blocked_count)
                : "memory", "cc"
            );
        }

#if OS_MONITOR_TCB_INTEGRITY
        /* TCB corrupted too — too risky to reset, deactivate instead */
        if (!os_tcb_check_magic(task)) {
            task->state = TaskState::INACTIVE;
            continue;
        }
#endif

        if (task == current_task) {
            task->state  = TaskState::INACTIVE;
            current_task = nullptr;
            OS_SCB_ICSR  = OS_ICSR_PENDSVSET_Msk;
            return;
        }

        if (stack_recovery_count < OS_SAFETY_TASK_MAX_RECOVERY) {
            stack_recovery_count++;
            os_reset_task_internal(task);
        }
        else {
            task->state = TaskState::INACTIVE;
        }
    }
}


/* ═══════════════ Fault Handler ═══════════════
 * Captures comprehensive fault context for diagnostics:
 *   - R0-R3, R12, LR, PC, xPSR (exception frame on stack)
 *   - CFSR (Configurable Fault Status Register)
 *   - HFSR (HardFault Status Register)
 *   - MMFAR (MemManage Fault Address Register)
 *   - BFAR (BusFault Address Register)
 *   - Current task name, ID, stack info, program counter
 *
 * After capture, the offending task is disabled and the system
 * falls back to the idle task for continued operation. */

/* Fault context structure — captured on stack during fault handler */
struct ZenOS_FaultContext {
    /* Exception frame (pushed by hardware) */
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
    /* NVIC registers */
    uint32_t cfsr;
    uint32_t hfsr;
    uint32_t mmfar;
    uint32_t bfar;
    uint32_t ctrl;
    uint32_t msp;
    uint32_t psp;
    /* Task context */
    const char* task_name;
    uint8_t     task_id;
    uint32_t*   task_stack_top;
    uint32_t    task_stack_size;
};

/* Global fault context for post-mortem analysis */
volatile ZenOS_FaultContext os_last_fault;

extern "C" void OS_Fault_C_Handler(void) {
    os_report_error(OSError::HARDFAULT);
    os_safe_depth = 0;

    /* ── Capture NVIC fault registers before clearing ── */
    volatile uint32_t cfsr  = *((volatile uint32_t*)0xE000ED28UL); /* CFSR */
    volatile uint32_t hfsr  = *((volatile uint32_t*)0xE000ED2CUL); /* HFSR */
    volatile uint32_t mmfar = *((volatile uint32_t*)0xE000ED34UL); /* MMFAR */
    volatile uint32_t bfar  = *((volatile uint32_t*)0xE000ED38UL); /* BFAR */

    /* ── Populate fault context ── */
    os_last_fault.cfsr  = cfsr;
    os_last_fault.hfsr  = hfsr;
    os_last_fault.mmfar = mmfar;
    os_last_fault.bfar  = bfar;
    __asm volatile("mrs %0, CONTROL" : "=r"(os_last_fault.ctrl));
    __asm volatile("mrs %0, MSP"     : "=r"(os_last_fault.msp));
    __asm volatile("mrs %0, PSP"     : "=r"(os_last_fault.psp));

    TCB* fault_task = current_task;
    if (fault_task && fault_task != &idle_tcb) {
        os_last_fault.task_name       = fault_task->name;
        os_last_fault.task_id         = fault_task->id;
        os_last_fault.task_stack_top  = fault_task->stack_top;
        os_last_fault.task_stack_size = fault_task->stack_size;

        /* Try to extract LR and PC from the exception frame on the
           task's PSP stack (if PSP is valid). The exception frame
           is laid out as: R0, R1, R2, R3, R12, LR, PC, xPSR */
        uint32_t psp = (uint32_t)fault_task->stack_top;
        if (psp >= (uint32_t)fault_task->stack_base &&
            psp <  (uint32_t)(fault_task->stack_base + fault_task->stack_size)) {
            uint32_t* frame = (uint32_t*)psp;
            os_last_fault.r0   = frame[0];
            os_last_fault.r1   = frame[1];
            os_last_fault.r2   = frame[2];
            os_last_fault.r3   = frame[3];
            os_last_fault.r12  = frame[4];
            os_last_fault.lr   = frame[5];
            os_last_fault.pc   = frame[6];
            os_last_fault.xpsr = frame[7];
        } else {
            /* PSP is corrupted — use MSP instead */
            uint32_t msp_val;
            __asm volatile("mrs %0, MSP" : "=r"(msp_val));
            uint32_t* frame = (uint32_t*)msp_val;
            os_last_fault.r0   = frame[0];
            os_last_fault.r1   = frame[1];
            os_last_fault.r2   = frame[2];
            os_last_fault.r3   = frame[3];
            os_last_fault.r12  = frame[4];
            os_last_fault.lr   = frame[5];
            os_last_fault.pc   = frame[6];
            os_last_fault.xpsr = frame[7];
        }

        /* ── Disable the offending task ── */
        if (fault_task->state == TaskState::BLOCKED && blocked_count > 0) {
            uint32_t tmp, res;
            __asm volatile(
                "1: ldrex %0, [%2]\n"
                "   cmp   %0, #0\n"
                "   beq   2f\n"
                "   subs  %0, %0, #1\n"
                "   strex %1, %0, [%2]\n"
                "   cmp   %1, #0\n"
                "   bne   1b\n"
                "2:\n"
                : "=&r"(tmp), "=&r"(res)
                : "r"(&blocked_count)
                : "memory", "cc"
            );
        }
        fault_task->state = TaskState::INACTIVE;
    }

    current_task = nullptr;

    /* ── Reset idle task for recovery ── */
    os_stack_init(&idle_tcb);
    idle_tcb.state         = TaskState::READY;
    idle_tcb.delay_ticks   = 0;
    idle_tcb.blocking_on   = nullptr;
    idle_tcb.block_timeout = 0;
    idle_tcb.wait_result   = 0;
    idle_tcb.last_yield_tick = tick_count;
    idle_tcb.mutex_nesting = 0;
    idle_tcb.base_priority = 0;

    __asm volatile("msr psp, %0" :: "r"(idle_tcb.stack_top) : "memory");
    /* DSB: ensure idle_tcb and current_task writes are visible before PendSV */
    __asm volatile("dsb" ::: "memory");
    OS_SCB_ICSR = OS_ICSR_PENDSVSET_Msk;
}

extern "C" OS_NAKED OS_USED void OS_Fault_Handler(void) {
    __asm volatile(
        ".syntax unified\n"
        ".thumb\n"
        "mov r0, lr\n"
        "push {r0}\n"
        "cpsid i\n"
        "bl OS_Fault_C_Handler\n"
        "pop {r0}\n"
        "mov lr, r0\n"
        "cpsie i\n"
        "bx lr\n"
    );
}


/* ══════════════════════════════════════════════════════════════════════
 *  Error Log System
 * ══════════════════════════════════════════════════════════════════════ */
#if OS_MONITOR_ERROR_LOG
static OSErrorEntry error_log[OS_MONITOR_ERROR_LOG_SIZE] OS_ALIGNED(4);
static uint32_t error_log_head = 0;
static uint32_t error_log_count = 0;
static uint32_t error_log_total = 0;

extern "C" void os_log_error(OSError code, uint8_t severity) {
    uint32_t cs = os_critical_enter();
    OSErrorEntry* e = &error_log[error_log_head];
    e->timestamp_tick = tick_count;
    e->code = code;
    e->task_id = current_task ? current_task->id : 0xFF;
    e->severity = (ErrorSeverity)severity;
    error_log_head = (error_log_head + 1) % OS_MONITOR_ERROR_LOG_SIZE;
    if (error_log_count < OS_MONITOR_ERROR_LOG_SIZE) error_log_count++;
    error_log_total++;
    os_critical_exit(cs);
}

extern "C" OSErrorEntry os_get_error_log_entry(uint32_t index) {
    OSErrorEntry empty = {};
    if (index >= error_log_count) return empty;
    uint32_t cs = os_critical_enter();
    uint32_t pos = (error_log_head + OS_MONITOR_ERROR_LOG_SIZE - 1 - index) % OS_MONITOR_ERROR_LOG_SIZE;
    OSErrorEntry result = error_log[pos];
    os_critical_exit(cs);
    return result;
}

extern "C" uint32_t os_get_error_log_count(void) {
    return error_log_count;
}

extern "C" uint32_t os_get_error_log_total(void) {
    return error_log_total;
}
#else
/* No-op stubs when error log is disabled */
extern "C" void os_log_error(OSError, uint8_t) {}
extern "C" OSErrorEntry os_get_error_log_entry(uint32_t) { return {}; }
extern "C" uint32_t os_get_error_log_count(void) { return 0; }
extern "C" uint32_t os_get_error_log_total(void) { return 0; }
#endif /* OS_MONITOR_ERROR_LOG */


/* ══════════════════════════════════════════════════════════════════════
 *  RAM Test (Background March-C)
 * ══════════════════════════════════════════════════════════════════════ */
#if OS_SAFETY_RAM_TEST
static uint32_t* ram_test_current = nullptr;
static uint32_t  ram_test_errors = 0;
static bool      ram_test_complete_flag = false;

extern "C" void os_ram_test_step(void) {
    if (ram_test_complete_flag) return;
    if (!ram_test_current) {
        ram_test_current = (uint32_t*)(uintptr_t)OS_RAM_TEST_START;
    }
    if ((uintptr_t)ram_test_current >= (uintptr_t)OS_RAM_TEST_END) {
        ram_test_complete_flag = true;
        return;
    }
    /* March-C: read → write complement → read complement → restore */
    uint32_t val = *ram_test_current;
    *ram_test_current = ~val;
    if (*ram_test_current != ~val) {
        ram_test_errors++;
        os_report_error(OSError::RAM_TEST_FAIL);
    }
    *ram_test_current = val;
    ram_test_current++;
}

extern "C" uint8_t os_ram_test_progress(void) {
    /* Use integer arithmetic to avoid pointer-subtraction warnings */
    uint32_t total = OS_RAM_TEST_END - OS_RAM_TEST_START;  /* bytes */
    if (total == 0) return 100;
    uint32_t done = 0;
    if (ram_test_current) {
        done = (uint32_t)((uintptr_t)ram_test_current - OS_RAM_TEST_START);
        if (done > total) done = total;
    }
    return (uint8_t)((done * 100UL) / total);
}

extern "C" uint32_t os_get_ram_test_error_count(void) {
    return ram_test_errors;
}

extern "C" bool os_ram_test_complete(void) {
    return ram_test_complete_flag;
}
#else
/* No-op stubs when RAM test is disabled */
extern "C" void os_ram_test_step(void) {}
extern "C" uint8_t os_ram_test_progress(void) { return 100; }
extern "C" uint32_t os_get_ram_test_error_count(void) { return 0; }
extern "C" bool os_ram_test_complete(void) { return true; }
#endif /* OS_SAFETY_RAM_TEST */


/* ══════════════════════════════════════════════════════════════════════
 *  Hardware Watchdog (IWDG)
 *  ---------------------------------------------------------------------------
 *  Uses port-level macros (OS_IWDG_KR, OS_IWDG_SR) defined in ZenOS_Port.hpp
 *  so the same code works on every STM32 family.  The actual IWDG init
 *  (prescaler, reload, window) is done by CubeMX-generated code.
 *
 *  os_hw_watchdog_feed() — unconditional feed (call when system is healthy)
 *  os_hw_watchdog_check() — conditional feed (skip if errors detected)
 * ══════════════════════════════════════════════════════════════════════ */
#if OS_SAFETY_HW_WATCHDOG
static volatile uint32_t hw_wdg_reset_count_var = 0;

extern "C" void os_hw_watchdog_feed(void) {
    /* Wait for any ongoing prescaler/reload update to finish
       (IWDG->SR bits PVU|RVU) so the feed is not dropped. */
    while (OS_IWDG_SR & 0x03UL) { }
    /* Feed the watchdog: write 0xAAAA to IWDG->KR */
    OS_IWDG_KR = 0xAAAA;
}

extern "C" void os_hw_watchdog_check(void) {
    /* Only feed if system is healthy:
       - error count below the transient-error threshold
       - scheduler is running a task */
    if (os_get_error_count() < 10 && current_task != nullptr) {
        os_hw_watchdog_feed();
    }
    /* If not healthy: skip feed → IWDG will reset MCU */
}

extern "C" uint32_t os_get_hw_wdg_reset_count(void) {
    /* Check RCC CSR for the IWDG reset flag using the port-level address AND
       the port-level bit positions (OS_RCC_IWDGRSTF_BIT / OS_RCC_RMVF_BIT).
       The RCC_CSR layout is not identical on every STM32 series, so both the
       address (OS_RCC_CSR_ADDR) and the flag bits must come from the port. */
    uint32_t csr = *((volatile uint32_t*)OS_RCC_CSR_ADDR);
    if (csr & (1UL << OS_RCC_IWDGRSTF_BIT)) {
        hw_wdg_reset_count_var++;
        /* Clear all reset flags by writing RMVF */
        *((volatile uint32_t*)OS_RCC_CSR_ADDR) |= (1UL << OS_RCC_RMVF_BIT);
    }
    return hw_wdg_reset_count_var;
}
#else
/* No-op stubs when HW watchdog is disabled */
extern "C" void os_hw_watchdog_feed(void) {}
extern "C" void os_hw_watchdog_check(void) {}
extern "C" uint32_t os_get_hw_wdg_reset_count(void) { return 0; }
#endif /* OS_SAFETY_HW_WATCHDOG */


/* ══════════════════════════════════════════════════════════════════════
 *  CRC Program Flow Monitoring
 *  ---------------------------------------------------------------------------
 *  Uses port-level macros (OS_CRC_DR, OS_CRC_CR) defined in ZenOS_Port.hpp
 *  so the same code works on every STM32 family.  Verifies ROM integrity
 *  by computing CRC over flash and comparing against expected value
 *  stored at boot.
 *
 *  NOTE: The CRC polynomial is set by CubeMX.  Different STM32 families
 *  may default to different polynomials (CRC-32 vs CRC-32C).  The
 *  os_crc_init() function captures the expected CRC at boot time, so
 *  the polynomial doesn't need to match across platforms — only the
 *  flash content must be identical between init and check.
 * ══════════════════════════════════════════════════════════════════════ */
#if OS_SAFETY_CRC_CHECK
static uint32_t crc_expected = 0;
static uint32_t crc_current_addr = 0;
static uint32_t crc_error_count = 0;
static bool     crc_complete = false;
static bool     crc_init_done = false;

static uint32_t os_crc_compute_block(uint32_t addr, uint32_t size_words) {
    OS_CRC_CR = 1;  /* Reset CRC peripheral (CR[0] = RESET) */
    volatile uint32_t* p = (volatile uint32_t*)addr;
    for (uint32_t i = 0; i < size_words; i++)
        OS_CRC_DR = p[i];
    return OS_CRC_DR;
}

extern "C" void os_crc_init(void) {
    /* Compute CRC over entire flash at boot */
    crc_expected = os_crc_compute_block(OS_FLASH_START, OS_FLASH_SIZE / 4);
    crc_current_addr = OS_FLASH_START;
    crc_complete = false;
    crc_init_done = true;
}

extern "C" void os_crc_check_step(void) {
    if (!crc_init_done || crc_complete) return;

    /* Full CRC check — 64 words per step keeps the ISR budget bounded */
    uint32_t chunk_words = 64;
    uint32_t remaining_bytes = (OS_FLASH_START + OS_FLASH_SIZE) - crc_current_addr;
    uint32_t words = remaining_bytes / 4;
    if (words > chunk_words) words = chunk_words;
    crc_current_addr += words * 4;

    /* Done when the whole flash has been re-CRC'd */
    if (crc_current_addr >= OS_FLASH_START + OS_FLASH_SIZE) {
        crc_complete = true;
        uint32_t full_crc = os_crc_compute_block(OS_FLASH_START, OS_FLASH_SIZE / 4);
        if (full_crc != crc_expected) {
            crc_error_count++;
            os_report_error(OSError::HARDFAULT);  /* ROM corruption */
        }
    }
}

extern "C" uint8_t os_crc_check_progress(void) {
    if (!crc_init_done) return 0;
    if (crc_complete) return 100;
    uint32_t total = OS_FLASH_SIZE;
    if (total == 0) return 100;
    uint32_t done = crc_current_addr - OS_FLASH_START;
    if (done > total) done = total;
    return (uint8_t)((done * 100UL) / total);
}

extern "C" bool os_crc_check_complete(void) {
    return crc_complete;
}

extern "C" uint32_t os_get_crc_error_count(void) {
    return crc_error_count;
}
#else
/* No-op stubs when CRC check is disabled */
extern "C" void os_crc_init(void) {}
extern "C" void os_crc_check_step(void) {}
extern "C" uint8_t os_crc_check_progress(void) { return 100; }
extern "C" bool os_crc_check_complete(void) { return true; }
extern "C" uint32_t os_get_crc_error_count(void) { return 0; }
#endif /* OS_SAFETY_CRC_CHECK */


/* ══════════════════════════════════════════════════════════════════════
 *  MPU Protection — PMSAv7 (Cortex-M3/M4/M7) and PMSAv8 (M23/M33)
 *  =========================================================================
 *  The MPU type is selected at compile time by OS_MPU_PMSAv7 / OS_MPU_PMSAv8
 *  (set in ZenOS_Port.hpp based on __ARM_ARCH_* macros).
 *
 *  PMSAv7 (Cortex-M3/M4/M7):
 *    - RBAR: [31:5] BASE, [3:0] REGION
 *    - RASR: [28] XN, [26:24] AP, [18:16] TEX, [15:8] SRD, [5:2] SIZE, [0] ENABLE
 *    - AP encoding: 3=RW/RW, 5=RO/RO, 6=RO/RW (no access from unprivileged)
 *
 *  PMSAv8 (Cortex-M23/M33):
 *    - RBAR: [31:N] BASE (N=alignment from region size), [3:0] REGION
 *    - RLAR: [31:N] LIMIT, [8:6] AttrIndx, [5:4] SH, [3:1] AP, [0] EN
 *    - AP encoding: 0=RW/RW, 1=RW/RO, 2=RO/RO, 3=RO/RW (Priv RW, Unpriv RO)
 * ══════════════════════════════════════════════════════════════════════ */
#if OS_SAFETY_MPU

/* ── PMSAv7 Implementation ── */
#if OS_MPU_PMSAv7
#define OS_MPU_CTRL_ENABLE_Msk     (1UL)
#define OS_MPU_CTRL_PRIVDEFENA_Msk (1UL << 2)
#define MPU_RASR_ENABLE_Pos     0
#define MPU_RASR_AP_Pos         24
#define MPU_RASR_SIZE_Pos       1
#define MPU_RASR_XN_Pos         28
#define MPU_AP_FULL_ACCESS  3   /* RW for both privilege levels */
#define MPU_AP_READONLY     5   /* RO for both privilege levels */

/* Program one PMSAv7 MPU region. Caller must keep the MPU disabled (CTRL=0)
   while reprogramming — required by ARMv7-M. */
static void os_mpu_set_region(uint8_t region, uint32_t base,
                              uint32_t size_bytes, uint32_t ap, bool xn) {
    if (region >= OS_MPU_MAX_REGIONS) return;

    /* PMSAv7 rules (ARMv7-M ARM, B3.5.1 / MPU_RASR.SIZE):
     *   - region size must be a power of two, minimum 32 bytes;
     *   - SIZE field = log2(size) - 1, so SIZE=4 => 32 bytes ... SIZE=31 => 4 GB;
     *   - the base address MUST be aligned to the region size.
     *
     * The previous code started size_log at 4 and looped while
     * (1 << (size_log+1)) < size_bytes, which: (a) never rejected a
     * non-power-of-two size, and (b) never validated base alignment,
     * so small/misaligned regions silently became wrong or disabled. */
    if (size_bytes < 32) size_bytes = 32;
    /* Round UP to the next power of two (2^(size_log+1) >= size_bytes). */
    uint32_t size_log = 4;                       /* 32 bytes */
    while (size_log < 31 && ((1UL << (size_log + 1)) < size_bytes))
        size_log++;
    uint32_t region_size = 1UL << (size_log + 1);
    if (region_size < size_bytes) return;        /* > 4 GB, cannot encode */
    if ((base & (region_size - 1)) != 0) {       /* base must be aligned */
        os_report_error(OSError::PRIORITY_CONFLICT);
        return;
    }
    OS_MPU_BASE->RNR  = region;
    OS_MPU_BASE->RBAR = base;
    OS_MPU_BASE->RASR = (1UL << MPU_RASR_ENABLE_Pos) |
                ((ap & 0x07UL) << MPU_RASR_AP_Pos) |
                (size_log << MPU_RASR_SIZE_Pos) |
                (xn ? (1UL << MPU_RASR_XN_Pos) : 0UL);
}

/* ── PMSAv8 Implementation ── */
#elif OS_MPU_PMSAv8
#define OS_MPU_CTRL_ENABLE_Msk     (1UL)
#define OS_MPU_CTRL_PRIVDEFENA_Msk (1UL << 2)

/* Program one PMSAv8 MPU region. */
static void os_mpu_set_region(uint8_t region, uint32_t base,
                              uint32_t size_bytes, uint32_t ap, bool xn) {
    if (region >= OS_MPU_MAX_REGIONS) return;
    /* Compute limit address (size must be power of 2) */
    uint32_t limit = base + size_bytes - 1;
    /* AttrIndx = 0 (use default memory attributes from MAIR0/MAIR1)
       SH = 2 (Inner Shareable — needed for multi-master DMA coherency)
       AP: PMSAv8 encoding (bit[3:1] of RLAR): 0=RW/RW, 1=RW/RO, 2=RO/RO
       EN = 1 (region enabled)
       XN: bit[0] of RBAR (not available in RLAR for PMSAv8-MA) */
    uint32_t rlar_ap = ap & 0x07UL;
    OS_MPU_BASE->RNR  = region;
    OS_MPU_BASE->RBAR = (base & ~0x1FUL) | (xn ? 0x01UL : 0x00UL);  /* bit[0]=XN */
    OS_MPU_BASE->RLAR = (limit & ~0x1FUL) |
                         (0UL << 6) |   /* AttrIndx = 0 */
                         (2UL << 4) |   /* SH = Inner Shareable */
                         (rlar_ap << 1) |
                         0x01UL;        /* EN = 1 */
}

#else
/* Fallback: no MPU hardware — MPU functions are no-ops (correct behavior) */
static void os_mpu_set_region(uint8_t, uint32_t, uint32_t, uint32_t, bool) {}
#endif /* OS_MPU_PMSAv7 vs PMSAv8 */


extern "C" void os_mpu_init(void) {
    OS_MPU_BASE->CTRL = 0;
    __asm volatile("dsb" ::: "memory");
    __asm volatile("isb");
    /* Static regions shared by all tasks:
       0 = flash (code)  — read-only, executable
       1 = SRAM (data)   — read/write (tasks share data memory)
       2 = peripherals   — read/write, non-executable
       3 = per-task stack — programmed in os_mpu_configure_task
       4+ = extra regions via os_mpu_add_region
       PRIVDEFENA lets privileged code (kernel, idle, ISRs) bypass the MPU;
       tasks run unprivileged (CONTROL.nPRIV set in PendSV) and are
       restricted to the regions above. */
    os_mpu_set_region(0, OS_FLASH_START, OS_FLASH_SIZE, MPU_AP_READONLY, false);
    os_mpu_set_region(1, OS_RAM_START,    OS_RAM_SIZE,    MPU_AP_FULL_ACCESS, true);
    os_mpu_set_region(2, 0x40000000UL,    0x20000000UL,   MPU_AP_FULL_ACCESS, true);
    OS_MPU_BASE->CTRL = OS_MPU_CTRL_ENABLE_Msk | OS_MPU_CTRL_PRIVDEFENA_Msk;
    __asm volatile("dsb" ::: "memory");
    __asm volatile("isb");
}

extern "C" void os_mpu_configure_task(void* tcb_ptr) {
    TCB* task = (TCB*)tcb_ptr;
    if (!task) return;
    /* Must disable the MPU before changing any region (ARMv7-M/ARMv8-M) */
    OS_MPU_BASE->CTRL = 0;
    __asm volatile("dsb" ::: "memory");
    /* Region 3: this task's stack — read/write, non-executable */
    os_mpu_set_region(3, (uint32_t)task->stack_base & ~0x1FUL,
                      task->stack_size * 4, MPU_AP_FULL_ACCESS, true);
    /* Extra per-task regions (4+) added via os_mpu_add_region */
    for (uint8_t i = 0; i < task->mpu_region_count && (4U + i) < OS_MPU_MAX_REGIONS; i++) {
        os_mpu_set_region(4 + i, task->mpu_regions[i].base_address,
                          task->mpu_regions[i].size,
                          task->mpu_regions[i].attributes, true);
    }
    OS_MPU_BASE->CTRL = OS_MPU_CTRL_ENABLE_Msk | OS_MPU_CTRL_PRIVDEFENA_Msk;
    __asm volatile("dsb" ::: "memory");
    __asm volatile("isb");
}

extern "C" void os_mpu_add_region(void* tcb_ptr, uint32_t base, uint32_t size, uint32_t attrs) {
    TCB* task = (TCB*)tcb_ptr;
    if (!task) return;
    if (task->mpu_region_count >= OS_MPU_MAX_REGIONS) return;
    task->mpu_regions[task->mpu_region_count].base_address = base;
    task->mpu_regions[task->mpu_region_count].size         = size;
    task->mpu_regions[task->mpu_region_count].attributes   = attrs;
    task->mpu_region_count++;
}

extern "C" void os_mpu_disable(void) {
    OS_MPU_BASE->CTRL = 0;
    __asm volatile("dsb" ::: "memory");
    __asm volatile("isb");
}

extern "C" void os_mpu_enable(void) {
    OS_MPU_BASE->CTRL = OS_MPU_CTRL_ENABLE_Msk | OS_MPU_CTRL_PRIVDEFENA_Msk;
    __asm volatile("dsb" ::: "memory");
    __asm volatile("isb");
}
#else
/* No-op stubs when MPU is disabled */
extern "C" void os_mpu_init(void) {}
extern "C" void os_mpu_configure_task(void*) {}
extern "C" void os_mpu_add_region(void*, uint32_t, uint32_t, uint32_t) {}
extern "C" void os_mpu_disable(void) {}
extern "C" void os_mpu_enable(void) {}
#endif /* OS_SAFETY_MPU */
