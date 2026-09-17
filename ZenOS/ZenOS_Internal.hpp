#pragma once
/**
 * @file    ZenOS_Internal.hpp
 * @brief   ZenOS RTOS — Internal shared declarations between modules
 *
 * This header is NOT part of the public API. It provides extern
 * declarations for functions and globals shared across the split
 * translation units (scheduler, IPC, safety, monitor).
 *
 * ALL functions and variables here use the _os_ prefix to mark them
 * as internal — they are NOT for application use.
 *
 * @author  Rahman Heidari <rahman.h22@gmail.com> — Raymon Research Team
 * @version 1.1.0
 */

#define OS_BUILD
#include "ZenOS.hpp"

/* ═══════════════ Shared Globals (internal) ═══════════════ */
extern TCB*              volatile _os_task_list;
extern uint16_t          volatile _os_task_count;
extern TCB*              volatile current_task;
extern volatile uint32_t tick_count;
extern volatile uint32_t os_safe_depth;

extern uint32_t          _os_idle_stack[];
extern TCB               _os_idle_tcb;
extern "C" TCB* const    os_idle_tcb_ptr;

extern volatile uint32_t _os_blocked_count;
extern volatile uint32_t _os_idle_ticks;  /* defined in ZenOS.cpp */
extern volatile bool     _os_started;

extern volatile uint32_t _os_error_total;
extern volatile OSError  _os_error_last;
extern volatile uint32_t _os_error_expected;
extern volatile uint32_t _os_error_expect_depth;
extern volatile uint32_t _os_wdg_reset_count;
extern volatile uint32_t _os_stack_recovery_count;

/* The first 32 priorities keep the original scalar bitmap and queue-head
   storage, so the default configuration has no additional scheduler RAM.
   Higher priorities use extension storage compiled only when requested. */
extern volatile uint32_t os_ready_bitmap;
extern TCB* volatile     os_pq_head[32];
#if OS_KERNEL_MAX_PRIORITIES > 32
extern volatile uint32_t _os_ready_bitmap_ext[OS_PRIORITY_EXTRA_WORDS];
extern TCB* volatile     _os_pq_head_ext[OS_PRIORITY_EXTRA_COUNT];
#endif

extern volatile uint32_t _os_syst_rvr_normal;

#if (OS_IPC_TOOLS)
extern ECB* volatile     _os_event_list;
extern int16_t           os_event_next_id;
/* ═══════════════ IPC Functions (internal) ═══════════════ */
extern "C" void _os_event_signal(int16_t id);
extern "C" void _os_event_signal_from_isr(uint32_t mask);
#endif

/* ═══════════════ Scheduler Functions (internal) ═══════════════ */
void _os_pq_add(TCB* task);
void _os_pq_remove(TCB* task);
extern "C" void _os_priority_queues_init(void);

void _os_stack_init(TCB* task);
void _os_reset_task_internal(TCB* task);
void _os_task_exit(void);

void _os_wake_task(TCB* t, uint8_t result);
bool _os_block_current(TaskState state, void* blocking_on,
                       uint32_t block_timeout, uint32_t delay_ticks);

inline void _os_wake_on_delay_expiry(TCB* task) {
    task->next_run_time = tick_count;
    _os_wake_task(task, 1);
}

inline void _os_wake_on_timeout_expiry(TCB* task) {
    task->wait_result = 2;
    task->next_run_time = tick_count;
    _os_wake_task(task, 2);
}

uint32_t _os_ms_to_ticks(uint32_t ms);
TCB* _os_find_task_by_entry(void(*entry)(void));
TCB* _os_find_task_by_id(uint8_t id);

/* ═══════════════ Microsecond Time Extension (internal) ═══════════════ */
extern volatile uint32_t _os_us_cycles_pending;
extern volatile uint32_t _os_us_remainder;
extern volatile uint32_t _os_us_accumulated;
void _os_time_reset(void);
void _os_time_sample(void);

#if OS_HAS_CYCLE_COUNTER
inline void _os_time_fold(void) {
    uint32_t cs  = os_critical_enter();
    uint32_t now = OS_DWT_CYCCNT;
    uint32_t delta = now - _os_us_cycles_pending;
    _os_us_cycles_pending = now;
    uint64_t units = (uint64_t)delta * 1000000ULL + _os_us_remainder;
    uint32_t clk   = (SystemCoreClock != 0UL) ? SystemCoreClock : 1UL;
    _os_us_accumulated += (uint32_t)(units / clk);
    _os_us_remainder    = (uint32_t)(units % clk);
    os_critical_exit(cs);
}
#else
inline void _os_time_fold(void) { /* no DWT — tick-derived domain */ }
#endif

#if OS_KERNEL_TICKLESS_IDLE
bool _os_tickless_process(uint32_t skip);
#endif

/* ═══════════════ FPU Detection Helper (internal) ═══════════════ */
#if OS_HAS_FPU_HW
inline bool _os_fpu_is_active() {
    uint32_t exc_return;
    __asm volatile("mov %0, lr" : "=r"(exc_return));
    return (exc_return & 0x10) == 0;
}
#endif

/* ═══════════════ ISR Detection (internal) ═══════════════ */
inline bool _os_in_isr(void) {
    uint32_t ipsr;
    __asm volatile("mrs %0, IPSR" : "=r"(ipsr));
    return ipsr != 0;
}

/* ═══════════════ Error Reporting (internal) ═══════════════ */
void _os_report_error(OSError code);

/* ═══════════════ Safety Functions (internal) ═══════════════ */
void _os_stack_check_all(void);

/* Fault context structure */
struct ZenOS_FaultContext {
    uint32_t r0, r1, r2, r3;
    uint32_t r12, lr, pc, xpsr;
    uint32_t cfsr, hfsr, mmfar, bfar;
    uint32_t ctrl, msp, psp;
    const char* task_name;
    uint8_t     task_id;
    uint32_t*   task_stack_top;
    uint32_t    task_stack_size;
};

extern volatile ZenOS_FaultContext os_last_fault;

#if OS_MONITOR_TCB_INTEGRITY
inline bool _os_tcb_check_magic(TCB* t) {
    return t && t->magic == OS_TCB_MAGIC;
}
#endif

/* ═══════════════ Monitor Functions (internal) ═══════════════ */
#if OS_MONITOR_ENABLED
uint32_t _os_stack_watermark_scan(const TCB* task);
#endif

/* ═══════════════ Critical Section ═══════════════ */
extern "C" uint32_t os_critical_enter(void);
extern "C" void os_critical_exit(uint32_t old);
