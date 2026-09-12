/**
 * @file    ZenOS_Scheduler.cpp
 * @brief   ZenOS RTOS — Configurable priority bitmap scheduler & task management
 *
 * Extracted from ZenOS.cpp as part of the modular split.
 * Priority slots are configured by OS_KERNEL_MAX_PRIORITIES (2..256), with
 * priority 0 reserved and usable priorities 1..OS_KERNEL_MAX_PRIORITIES-1.
 *
 * @author  Rahman Heidari <rahman.h22@gmail.com> — Raymon Research Team
 * @version 1.0.1
 */

#define OS_BUILD
#include "ZenOS_Internal.hpp"

#if OS_KERNEL_MAX_PRIORITIES > 32
volatile uint32_t os_ready_bitmap_ext[OS_PRIORITY_EXTRA_WORDS] = {0};
TCB* volatile os_pq_head_ext[OS_PRIORITY_EXTRA_COUNT] = {nullptr};
#endif

static inline bool os_priority_valid(uint8_t prio) {
    return prio > 0 && prio < OS_KERNEL_MAX_PRIORITIES;
}

static inline TCB* volatile* os_pq_head_for(uint8_t prio) {
    if (prio < 32) return &os_pq_head[prio];
#if OS_KERNEL_MAX_PRIORITIES > 32
    return &os_pq_head_ext[prio - 32];
#else
    return nullptr;
#endif
}

static inline void os_pq_set_bit(uint8_t prio) {
    if (!os_priority_valid(prio)) return;
    if (prio < 32) {
        os_ready_bitmap |= (1UL << prio);
        return;
    }
#if OS_KERNEL_MAX_PRIORITIES > 32
    uint8_t p = (uint8_t)(prio - 32);
    os_ready_bitmap_ext[p >> 5] |= (1UL << (p & 31));
#endif
}

static inline void os_pq_clear_bit(uint8_t prio) {
    if (!os_priority_valid(prio)) return;
    if (prio < 32) {
        os_ready_bitmap &= ~(1UL << prio);
        return;
    }
#if OS_KERNEL_MAX_PRIORITIES > 32
    uint8_t p = (uint8_t)(prio - 32);
    os_ready_bitmap_ext[p >> 5] &= ~(1UL << (p & 31));
#endif
}

static inline bool os_pq_bit_is_set(uint8_t prio) {
    if (!os_priority_valid(prio)) return false;
    if (prio < 32) return (os_ready_bitmap & (1UL << prio)) != 0;
#if OS_KERNEL_MAX_PRIORITIES > 32
    uint8_t p = (uint8_t)(prio - 32);
    return (os_ready_bitmap_ext[p >> 5] & (1UL << (p & 31))) != 0;
#else
    return false;
#endif
}

static inline uint8_t os_pq_highest_prio(void) {
#if OS_KERNEL_MAX_PRIORITIES > 32
    for (int word = (int)OS_PRIORITY_EXTRA_WORDS - 1; word >= 0; --word) {
        uint32_t bits = os_ready_bitmap_ext[word];
        if (bits != 0) {
            uint8_t bit = (uint8_t)(31UL - (uint32_t)__builtin_clz(bits));
            uint16_t prio = (uint16_t)(32U + (uint16_t)word * 32U + bit);
            if (prio < OS_KERNEL_MAX_PRIORITIES) return (uint8_t)prio;
        }
    }
#endif
    if (os_ready_bitmap != 0)
        return (uint8_t)(31UL - (uint32_t)__builtin_clz(os_ready_bitmap));
    return 0;
}

/* ═══════════════ Priority Queue Operations ═══════════════ */
void os_pq_add(TCB* task) {
    if (!task) return;
    /* Priority 0 is reserved: the idle task is NEVER enqueued and must not
       touch scheduler storage.  os_start() calls this with &idle_tcb AFTER
       all application tasks are queued — resetting the queues here (the old
       lifecycle-reset behavior) unlinked every READY task and stalled the
       scheduler at boot.  Queue reset belongs to os_init() and to
       _os_task_create_internal() when task_count == 0, which cover the
       previous-lifecycle cleanup this used to guard. */
    if (task == &idle_tcb) return;
    if (!os_priority_valid(task->priority)) return;
    uint8_t p = task->priority;
    TCB* volatile* head = os_pq_head_for(p);
    if (!head) return;
    task->queue_next = *head;
    *head = task;
    os_pq_set_bit(p);
}

void os_pq_remove(TCB* task) {
    if (!task || !os_priority_valid(task->priority)) return;
    uint8_t p = task->priority;
    TCB* volatile* pp = os_pq_head_for(p);
    if (!pp) return;
    while (*pp) {
        if (*pp == task) {
            *pp = task->queue_next;
            task->queue_next = nullptr;
            if (!*pp) os_pq_clear_bit(p);
            return;
        }
        pp = &(*pp)->queue_next;
    }
}

/* ── Round-Robin Throttle ── */
#if OS_KERNEL_MAX_PRIORITIES <= 32
static uint32_t os_rr_skip = 0;
#else
static uint32_t os_rr_skip = 0;
static uint32_t os_rr_skip_ext[OS_PRIORITY_EXTRA_WORDS] = {0};
#endif
static uint32_t os_rr_skip_quota = 0;

/* Reset all scheduler-owned priority state. Kept here so os_init() can reset
   extension storage without exposing the static RR arrays as globals. */
extern "C" void os_priority_queues_init(void) {
    os_ready_bitmap = 0;
    for (uint32_t i = 0; i < 32; ++i) os_pq_head[i] = nullptr;

#if OS_KERNEL_MAX_PRIORITIES > 32
    for (uint32_t i = 0; i < OS_PRIORITY_EXTRA_WORDS; ++i)
        os_ready_bitmap_ext[i] = 0;
    for (uint32_t i = 0; i < OS_PRIORITY_EXTRA_COUNT; ++i)
        os_pq_head_ext[i] = nullptr;
#endif
}

static inline bool os_rr_is_skipped(uint8_t prio) {
    if (!os_priority_valid(prio)) return false;
    if (prio < 32) return (os_rr_skip & (1UL << prio)) != 0;
#if OS_KERNEL_MAX_PRIORITIES > 32
    uint8_t p = (uint8_t)(prio - 32);
    return (os_rr_skip_ext[p >> 5] & (1UL << (p & 31))) != 0;
#else
    return false;
#endif
}

static inline void os_rr_set_skip(uint8_t prio) {
    if (prio < 32) os_rr_skip |= (1UL << prio);
#if OS_KERNEL_MAX_PRIORITIES > 32
	else if (prio < OS_KERNEL_MAX_PRIORITIES) {
        uint8_t p = (uint8_t)(prio - 32);
        os_rr_skip_ext[p >> 5] |= (1UL << (p & 31));
    }
#endif
}

static inline void os_rr_clear_all(void) {
    os_rr_skip = 0;
#if OS_KERNEL_MAX_PRIORITIES > 32
    for (uint32_t i = 0; i < OS_PRIORITY_EXTRA_WORDS; ++i)
        os_rr_skip_ext[i] = 0;
#endif
}

static inline uint32_t os_ready_level_count(void) {
    uint32_t count = (uint32_t)__builtin_popcount(os_ready_bitmap);
#if OS_KERNEL_MAX_PRIORITIES > 32
    for (uint32_t i = 0; i < OS_PRIORITY_EXTRA_WORDS; ++i)
        count += (uint32_t)__builtin_popcount(os_ready_bitmap_ext[i]);
#endif
    return count;
}

extern "C" TCB* os_pq_next(void) {
    if (os_pq_highest_prio() == 0) return nullptr;
    const uint32_t now = tick_count;

    for (int p = (int)OS_KERNEL_MAX_PRIORITIES - 1; p >= 1; --p) {
        uint8_t prio = (uint8_t)p;
        if (!os_pq_bit_is_set(prio)) continue;
        TCB* head = *os_pq_head_for(prio);
        if (!head) {
            os_pq_clear_bit(prio);
            continue;
        }

        TCB* prev = nullptr;
        TCB* selected = nullptr;
        for (TCB* t = head; t; t = t->queue_next) {
            bool eligible = (t->period_ticks == 0) ||
                            ((int32_t)(now - t->next_run_time) >= 0);
            if (eligible) {
                selected = t;
                break;
            }
            prev = t;
        }
        if (!selected) continue;

        if (selected != head) {
            prev->queue_next = selected->queue_next;
            selected->queue_next = head;
            *os_pq_head_for(prio) = selected;
        }
        return selected;
    }
    return nullptr;
}

extern "C" void os_pq_rotate(void) {
    if (os_pq_highest_prio() == 0) return;
    const uint32_t now = tick_count;
    uint8_t p = 0xFF;

    for (int prio = (int)OS_KERNEL_MAX_PRIORITIES - 1; prio >= 1; --prio) {
        uint8_t candidate = (uint8_t)prio;
        if (!os_pq_bit_is_set(candidate)) continue;
        for (TCB* t = *os_pq_head_for(candidate); t; t = t->queue_next) {
            if (t->period_ticks == 0 ||
                ((int32_t)(now - t->next_run_time) >= 0)) {
                p = candidate;
                break;
            }
        }
        if (p != 0xFF) break;
    }
    if (p == 0xFF) return;

    TCB* volatile* head_ptr = os_pq_head_for(p);
    TCB* head = *head_ptr;
    if (!head) return;

    /* Single task at this priority — no rotation needed, just return. */
    if (!head->queue_next) return;

    /* Multiple tasks: move head to tail for round-robin. */
    *head_ptr = head->queue_next;
    TCB* tail = *head_ptr;
    while (tail->queue_next) tail = tail->queue_next;
    tail->queue_next = head;
    head->queue_next = nullptr;
}

/* ═══════════════ Stack Init ═══════════════ */
void os_stack_init(TCB* task) {
    uint32_t* base = task->stack_base;
    uint32_t  size = task->stack_size;
    for (uint32_t i = OS_STACK_CANARY_COUNT; i < size; i++)
        base[i] = 0xA5A5A5A5UL;
    for (uint32_t i = 0; i < OS_STACK_CANARY_COUNT; i++)
        base[i] = OS_STACK_CANARY;

    uint32_t* sp = base + size;
    sp = (uint32_t*)((uint32_t)sp & ~0x7UL);
    uint32_t ctx[16];
    ctx[0]  = 0x01000000UL;
    ctx[1]  = (uint32_t)task->entry;
    ctx[2]  = (uint32_t)os_task_exit;
    ctx[3]  = 0x0000000CUL;
    ctx[4]  = 0x00000003UL; ctx[5] = 0x00000002UL;
    ctx[6]  = 0x00000001UL; ctx[7] = 0x00000000UL;
    ctx[8]  = 0x0000000BUL; ctx[9] = 0x0000000AUL;
    ctx[10] = 0x00000009UL; ctx[11] = 0x00000008UL;
    ctx[12] = 0x00000007UL; ctx[13] = 0x00000006UL;
    ctx[14] = 0x00000005UL; ctx[15] = 0x00000004UL;
    sp -= 16;
    for (int i = 0; i < 16; i++) sp[i] = ctx[15-i];
    task->stack_top = sp;
}

void os_reset_task_internal(TCB* task) {
    os_stack_init(task);
    os_pq_remove(task);
    task->state = TaskState::READY;
    task->delay_ticks = 0;
    task->blocking_on = nullptr;
    task->block_timeout = 0;
    task->wait_result = 0;
    task->next_run_time = tick_count;
    task->last_yield_tick = tick_count;
    task->priority = task->base_priority;
    task->mutex_nesting = 0;
    os_pq_add(task);
}

void os_task_exit(void) {
    uint32_t cs = os_critical_enter();
    if (current_task) current_task->state = TaskState::INACTIVE;
    os_critical_exit(cs);
    os_yield();
    while (1) { __asm volatile("nop"); }
}

void os_wake_task(TCB* t, uint8_t result) {
    t->wait_result = result;
    t->state = TaskState::READY;
    t->blocking_on = nullptr;
    t->block_timeout = 0;
    /* Do NOT reset next_run_time here — the PendSV handler sets it based on
       the task's period_ticks (periodic tasks get tick_count + period_ticks,
       non-periodic tasks get tick_count + 1).  Resetting it to tick_count
       here bypassed the period gate, causing periodic tasks to fire at their
       delay rate instead of their period rate. */
    t->last_yield_tick = tick_count;
    if (blocked_count > 0) {
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
    os_pq_add(t);
}

bool os_block_current(TaskState state, void* blocking_on,
                      uint32_t block_timeout, uint32_t delay_ticks) {
    if (!current_task) return false;
    os_pq_remove(current_task);
    current_task->state = state;
    current_task->blocking_on = blocking_on;
    current_task->block_timeout = block_timeout;
    current_task->delay_ticks = delay_ticks;
    current_task->wait_result = 0;
    current_task->last_yield_tick = tick_count;
    uint32_t tmp, res;
    __asm volatile(
        "1: ldrex %0, [%2]\n"
        "   adds  %0, %0, #1\n"
        "   strex %1, %0, [%2]\n"
        "   cmp   %1, #0\n"
        "   bne   1b\n"
        : "=&r"(tmp), "=&r"(res)
        : "r"(&blocked_count)
        : "memory", "cc"
    );
    return true;
}

uint32_t os_ms_to_ticks(uint32_t ms) {
    if (ms == OS_WAIT_FOREVER) return 0;
    if (ms <= (0xFFFFFFFFUL / OS_TICKS_PER_MS)) {
        uint32_t r = ms * OS_TICKS_PER_MS;
        return r ? r : 1;
    }
    return 0xFFFFFFFEUL;
}

extern "C" int8_t _os_task_create_internal(
    TCB* task,
    uint32_t* stack_mem,
    uint32_t stack_size,
    const char* name,
    void(*entry)(void),
    uint8_t priority,
    uint32_t period_ms)
{
    if (os_started) { os_report_error(OSError::TASK_AFTER_START); return -1; }
    if (!task || !stack_mem || !entry) return -1;
    if (os_find_task_by_entry(entry)) return -1;
    if (priority == 0) priority = 1;
    if (!os_priority_valid(priority)) return -1;
    if (stack_size < 64) stack_size = 64;

    /* os_init() clears task_count. Reset scheduler storage before the first
       task of a new lifecycle is inserted, so no stale >32-priority state
       can survive a reinitialization. */
    if (task_count == 0) os_priority_queues_init();

    task->id = task_count++;
    task->name = name;
    task->entry = entry;
    task->priority = priority;
    task->base_priority = priority;
    task->mutex_nesting = 0;

    uint64_t p64 = (uint64_t)period_ms * OS_TICKS_PER_MS;
    task->period_ticks = (p64 > 0xFFFFFFFEULL) ? 0xFFFFFFFEUL : (uint32_t)p64;
    task->next_run_time = 0;
    task->state = TaskState::READY;
    task->delay_ticks = 0;
    task->blocking_on = nullptr;
    task->block_timeout = 0;
    task->wait_result = 0;
    task->stack_size = stack_size;
    task->last_yield_tick = tick_count;
    task->wdg_retries = 0;
    task->cpu_ticks = 0;
#if OS_SMP_CORES > 1
    task->core_id = 0;
    task->_pad[0] = task->_pad[1] = task->_pad[2] = 0;
#endif
#if OS_MONITOR_ENABLED
    task->peak_sp = task->stack_top;
#endif
#if OS_MONITOR_TCB_INTEGRITY
    task->magic = OS_TCB_MAGIC;
    task->overflow_count = 0;
#endif
#if OS_SAFETY_MPU
    task->mpu_region_count = 0;
#endif

    uintptr_t addr = (uintptr_t)stack_mem;
#if OS_SAFETY_MPU
    addr = (addr + 31) & ~31;
#else
    addr = (addr + 7) & ~7;
#endif
    task->stack_base = (uint32_t*)addr;
    os_stack_init(task);
    task->next = task_list;
    task_list = task;
    os_pq_add(task);
    return (int8_t)task->id;
}

TCB* os_find_task_by_entry(void(*entry)(void)) {
    if (!entry) return nullptr;
    for (TCB* t = task_list; t; t = t->next)
        if (t->entry == entry) return t;
    return nullptr;
}

TCB* os_find_task_by_id(uint8_t id) {
    for (TCB* t = task_list; t; t = t->next)
        if (t->id == id) return t;
    return nullptr;
}

extern "C" void os_task_stop(void(*entry)(void)) {
    uint32_t cs = os_critical_enter();
    TCB* t = os_find_task_by_entry(entry);
    if (t && t != &idle_tcb) {
        os_pq_remove(t);
        if (t->state == TaskState::BLOCKED && blocked_count > 0) {
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
        t->state = TaskState::INACTIVE;
        t->delay_ticks = 0;
        t->blocking_on = nullptr;
        t->block_timeout = 0;
        if (t == current_task) OS_SCB_ICSR = OS_ICSR_PENDSVSET_Msk;
    }
    os_critical_exit(cs);
}

extern "C" void os_task_start(void(*entry)(void)) {
    uint32_t cs = os_critical_enter();
    TCB* t = os_find_task_by_entry(entry);
    if (t && t->state == TaskState::INACTIVE) {
        os_reset_task_internal(t);
        t->wdg_retries = 0;
        OS_SCB_ICSR = OS_ICSR_PENDSVSET_Msk;
    }
    os_critical_exit(cs);
}

extern "C" uint8_t os_task_get_state(void(*entry)(void)) {
    uint32_t cs = os_critical_enter();
    TCB* t = os_find_task_by_entry(entry);
    uint8_t result = t ? (uint8_t)t->state : 0xFF;
    os_critical_exit(cs);
    return result;
}

extern "C" bool os_task_isActive(void(*entry)(void)) {
    uint32_t cs = os_critical_enter();
    TCB* t = os_find_task_by_entry(entry);
    uint8_t result = t ? (uint8_t)t->state : 0xFF;
    os_critical_exit(cs);
    return ((result != (uint8_t)TaskState::INACTIVE) ? true : false);
}

extern "C" uint8_t os_get_task_priority(void(*entry)(void)) {
    uint32_t cs = os_critical_enter();
    TCB* t = os_find_task_by_entry(entry);
    uint8_t result = t ? t->priority : 0;
    os_critical_exit(cs);
    return result;
}

#if OS_TOOL_TICKLESS_IDLE
bool os_tickless_process(uint32_t skip) {
    tick_count += skip;
    bool woke = false;
    if (blocked_count > 0) {
        for (TCB* task = task_list; task; task = task->next) {
            if (task->state != TaskState::BLOCKED) continue;
            if (task->delay_ticks > 0) {
                if (task->delay_ticks <= skip) {
                    task->delay_ticks = 0;
                    os_wake_on_delay_expiry(task);
                    /* Verify: after wake, next_run_time should be tick_count
                       (set by os_wake_on_delay_expiry in our fix). The period
                       gate in os_pq_next will then see (tick_count - next_run_time)
                       >= 0 and consider the task eligible. PendSV will override
                       next_run_time based on period_ticks when the task runs. */
                    if (task->period_ticks > 0) {
                        /* Periodic task: next_run_time should be current tick_count
                           so it's immediately eligible. PendSV will set it to
                           tick_count + period_ticks when the task runs. */
                        if (task->next_run_time > tick_count) {
                            /* This would mean the task is ineligible — the period
                               gate would reject it. This is a bug we're fixing! */
                            task->next_run_time = tick_count;
                        }
                    }
                    woke = true;
                } else task->delay_ticks -= skip;
            } else if (task->block_timeout > 0) {
                if (task->block_timeout <= skip) {
                    task->block_timeout = 0;
                    os_wake_on_timeout_expiry(task);
                    /* Same verification as above for timeout expiry wakes */
                    if (task->period_ticks > 0 && task->next_run_time > tick_count) {
                        task->next_run_time = tick_count;
                    }
                    woke = true;
                } else task->block_timeout -= skip;
            }
        }
    }
    return woke;
}
#endif

extern "C" uint32_t os_get_tick(void) { return tick_count; }

/* ── Time getters ─────────────────────────────────────────────────────────
   os_get_ms() derives from the kernel tick, so it stays consistent with
   os_get_tick() (including tickless-idle catch-up ticks) and wraps after
   ~49.5 days (32-bit ms counter).

   os_get_us() — wrap-safe extension of the DWT cycle counter.
   CYCCNT is only 32-bit (wraps every ~59.6 s at 72 MHz), so a raw read
   cannot measure beyond one wrap period.  os_us_accumulated holds the µs
   folded in by os_time_fold() (called from os_tick, os_time_reset and the
   lazy path below); this function simply folds what is pending and returns
   the accumulator.  All state changes happen inside the critical section,
   so task, ISR and tickless-wake callers cannot interleave.

   SystemCoreClock changes: conversion uses the current SystemCoreClock at
   every fold, so a clock change automatically applies only to subsequent
   windows.  After any clock change (e.g. HAL_RCC_ClockConfig +
   SystemCoreClockUpdate), call os_time_reset() to resynchronize the domain
   with the counter. */
extern "C" uint32_t os_get_us(void) {
#if OS_HAS_CYCLE_COUNTER
    os_time_fold();          /* lazy sample — same path os_tick uses */
    return os_us_accumulated;
#else
    /* No DWT: scale the kernel tick (resolution = OS_KERNEL_TICK_PERIOD_US,
       wraps mod 2^32 µs like the ms counter above). */
    return (uint32_t)((uint64_t)tick_count * OS_KERNEL_TICK_PERIOD_US);
#endif
}

extern "C" uint32_t os_get_ms(void) {
    return tick_count / OS_TICKS_PER_MS;
}

extern "C" uint16_t os_get_task_count(void) { return task_count; }
extern "C" uint32_t os_get_version(void) { return OS_VERSION_PACKED; }
extern "C" const char* os_get_version_string(void) { return OS_VERSION_STRING; }
