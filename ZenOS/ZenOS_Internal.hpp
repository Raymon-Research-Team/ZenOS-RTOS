#pragma once
#define OS_BUILD
#include "ZenOS.hpp"

extern TCB* volatile task_list;
extern uint16_t volatile task_count;
extern TCB* volatile current_task;
extern volatile uint32_t tick_count;
extern volatile uint32_t os_safe_depth;
extern uint32_t idle_stack[];
extern TCB idle_tcb;
extern "C" TCB* const os_idle_tcb_ptr;
extern volatile uint32_t blocked_count;
extern volatile uint32_t idle_ticks;
extern volatile bool os_started;
extern volatile uint32_t error_total;
extern volatile OSError error_last;
extern volatile uint32_t error_expected;
extern volatile uint32_t error_expect_depth;
extern volatile uint32_t wdg_reset_count;
extern volatile uint32_t stack_recovery_count;
extern volatile uint32_t os_ready_bitmap;
extern TCB* volatile os_pq_head[32];
extern volatile uint32_t os_syst_rvr_normal;
#if OS_TOOL_EVENT
extern ECB* volatile event_list;
extern int16_t os_event_next_id;
#endif

void os_pq_add(TCB* task);
void os_pq_remove(TCB* task);
extern "C" void os_priority_queues_init(void);
void os_stack_init(TCB* task);
void os_reset_task_internal(TCB* task);
void os_task_exit(void);
void os_wake_task(TCB* t, uint8_t result);
bool os_block_current(TaskState state, void* blocking_on, uint32_t block_timeout, uint32_t delay_ticks);

inline void os_wake_on_delay_expiry(TCB* task) {
    task->next_run_time = tick_count;
    os_wake_task(task, 1);
}
inline void os_wake_on_timeout_expiry(TCB* task) {
    task->wait_result = 2;
    task->next_run_time = tick_count;
    os_wake_task(task, 2);
}
#if OS_DEBUG_PERIOD_GATE
inline bool os_verify_period_gate_after_wake(TCB* task, uint32_t current_tick) {
    return task && (int32_t)(current_tick - task->next_run_time) >= 0;
}
#endif

uint32_t os_ms_to_ticks(uint32_t ms);
TCB* os_find_task_by_entry(void(*entry)(void));
TCB* os_find_task_by_id(uint8_t id);

extern volatile uint32_t os_us_cycles_pending;
extern volatile uint32_t os_us_remainder;
extern volatile uint32_t os_us_accumulated;
void os_time_reset(void);
void os_time_sample(void);
#if OS_HAS_CYCLE_COUNTER
inline void os_time_fold(void) {
    uint32_t cs = os_critical_enter();
    uint32_t now = OS_DWT_CYCCNT;
    uint32_t delta = now - os_us_cycles_pending;
    os_us_cycles_pending = now;
    uint64_t units = (uint64_t)delta * 1000000ULL + os_us_remainder;
    uint32_t clk = SystemCoreClock ? SystemCoreClock : 1UL;
    os_us_accumulated += (uint32_t)(units / clk);
    os_us_remainder = (uint32_t)(units % clk);
    os_critical_exit(cs);
}
#else
inline void os_time_fold(void) {}
#endif
#if OS_TOOL_TICKLESS_IDLE
bool os_tickless_process(uint32_t skip);
#endif

inline bool os_in_isr(void) { uint32_t ipsr; __asm volatile("mrs %0, IPSR" : "=r"(ipsr)); return ipsr != 0U; }
void os_report_error(OSError code);
void os_stack_check_all(void);
#if OS_MONITOR_TCB_INTEGRITY
inline bool os_tcb_check_magic(TCB* t) { return t && t->magic == OS_TCB_MAGIC; }
#endif
#if OS_MONITOR_ENABLED
uint32_t os_stack_watermark_scan(const TCB* task);
#endif

extern "C" uint32_t os_critical_enter(void);
extern "C" void os_critical_exit(uint32_t old);

/* Single-core atomic primitives. Interrupt masking is the portable primitive
   here; it avoids requiring LDREX/STREX on every Cortex-M implementation. */
inline void os_atomic_inc(volatile uint32_t* v) { uint32_t cs=os_critical_enter(); ++(*v); os_critical_exit(cs); }
inline void os_atomic_dec_saturating(volatile uint32_t* v) { uint32_t cs=os_critical_enter(); if(*v) --(*v); os_critical_exit(cs); }

extern "C" TCB* os_pendsv_select_task(void);
extern "C" void os_pendsv_activate_task(TCB* task);
