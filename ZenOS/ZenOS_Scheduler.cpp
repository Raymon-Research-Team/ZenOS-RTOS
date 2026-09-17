/**
 * @file    ZenOS_Scheduler.cpp
 * @brief   ZenOS RTOS — Configurable priority bitmap scheduler & task management
 *
 * Extracted from ZenOS.cpp as part of the modular split.
 * Priority slots are configured by OS_KERNEL_MAX_PRIORITIES (2..256).
 * PRIORITY CONVENTION (CMSIS-RTOS2 / FreeRTOS / ThreadX): a LARGER number
 * means a HIGHER priority.  Priority 0 is the idle task (lowest); usable
 * priorities are 1..OS_KERNEL_MAX_PRIORITIES-1, and
 * OS_KERNEL_MAX_PRIORITIES-1 is the highest priority.
 *
 * @author  Rahman Heidari <rahman.h22@gmail.com> — Raymon Research Team
 * @version 1.1.0
 */

#define OS_BUILD
#include "ZenOS_Internal.hpp"

#if OS_KERNEL_MAX_PRIORITIES > 32
volatile uint32_t _os_ready_bitmap_ext[OS_PRIORITY_EXTRA_WORDS] = {0};
TCB* volatile _os_pq_head_ext[OS_PRIORITY_EXTRA_COUNT] = {nullptr};
#endif

static inline bool _os_priority_valid(uint8_t prio) {
    return prio > 0 && prio < OS_KERNEL_MAX_PRIORITIES;
}

static inline TCB* volatile* _os_pq_head_for(uint8_t prio) {
    if (prio < 32) return &os_pq_head[prio];
#if OS_KERNEL_MAX_PRIORITIES > 32
    return &_os_pq_head_ext[prio - 32];
#else
    return nullptr;
#endif
}

static inline void _os_pq_set_bit(uint8_t prio) {
    if (!_os_priority_valid(prio)) return;
    if (prio < 32) {
        os_ready_bitmap |= (1UL << prio);
        return;
    }
#if OS_KERNEL_MAX_PRIORITIES > 32
    uint8_t p = (uint8_t)(prio - 32);
    _os_ready_bitmap_ext[p >> 5] |= (1UL << (p & 31));
#endif
}

static inline void _os_pq_clear_bit(uint8_t prio) {
    if (!_os_priority_valid(prio)) return;
    if (prio < 32) {
        os_ready_bitmap &= ~(1UL << prio);
        return;
    }
#if OS_KERNEL_MAX_PRIORITIES > 32
    uint8_t p = (uint8_t)(prio - 32);
    _os_ready_bitmap_ext[p >> 5] &= ~(1UL << (p & 31));
#endif
}

static inline bool _os_pq_bit_is_set(uint8_t prio) {
    if (!_os_priority_valid(prio)) return false;
    if (prio < 32) return (os_ready_bitmap & (1UL << prio)) != 0;
#if OS_KERNEL_MAX_PRIORITIES > 32
    uint8_t p = (uint8_t)(prio - 32);
    return (_os_ready_bitmap_ext[p >> 5] & (1UL << (p & 31))) != 0;
#else
    return false;
#endif
}

static inline uint8_t _os_pq_highest_prio(void) {
#if OS_KERNEL_MAX_PRIORITIES > 32
    for (int word = (int)OS_PRIORITY_EXTRA_WORDS - 1; word >= 0; --word) {
        uint32_t bits = _os_ready_bitmap_ext[word];
        if (bits != 0) {
            uint32_t lz;
            __asm volatile("clz %0, %1" : "=r"(lz) : "r"(bits));
            uint16_t prio = (uint16_t)(32U + (uint16_t)word * 32U + (31U - lz));
            if (prio < OS_KERNEL_MAX_PRIORITIES) return (uint8_t)prio;
        }
    }
#endif
    if (os_ready_bitmap != 0) {
        uint32_t lz;
        __asm volatile("clz %0, %1" : "=r"(lz) : "r"(os_ready_bitmap));
        return (uint8_t)(31U - lz);
    }
    return 0;
}

/* ═══════════════ Priority Queue Operations ═══════════════ */
void _os_pq_add(TCB* task) {
    if (!task) return;
    /* Priority 0 is reserved: the idle task is NEVER enqueued and must not
       touch scheduler storage.  os_start() calls this with &_os_idle_tcb AFTER
       all application tasks are queued — resetting the queues here (the old
       lifecycle-reset behavior) unlinked every READY task and stalled the
       scheduler at boot.  Queue reset belongs to os_init() and to
       _os_task_create_internal() when _os_task_count == 0, which cover the
       previous-lifecycle cleanup this used to guard. */
    if (task == &_os_idle_tcb) return;
    if (!_os_priority_valid(task->priority)) return;
    uint8_t p = task->priority;
    TCB* volatile* head = _os_pq_head_for(p);
    if (!head) return;
    task->queue_next = *head;
    *head = task;
    _os_pq_set_bit(p);
}

void _os_pq_remove(TCB* task) {
    if (!task || !_os_priority_valid(task->priority)) return;
    uint8_t p = task->priority;
    TCB* volatile* pp = _os_pq_head_for(p);
    if (!pp) return;
    while (*pp) {
        if (*pp == task) {
            *pp = task->queue_next;
            task->queue_next = nullptr;
            /* The ready bit must reflect whether the QUEUE is empty, not
               whether the predecessor's link is null.  When the removed
               task is the tail, *pp becomes null while the head and the
               tasks before it are still linked — clearing the bit there
               starved every remaining task at that priority level
               (the scheduler only ever looks at priorities whose bit is
               set).  Re-read the priority head instead. */
            if (*_os_pq_head_for(p) == nullptr) _os_pq_clear_bit(p);
            return;
        }
        pp = &(*pp)->queue_next;
    }
}

/* ── Round-Robin Fairness ──────────────────────────────────────────
 * Round-robin between same-priority tasks is guaranteed by two mechanisms:
 *   1. os_pq_rotate() moves the selected task to the tail of its priority
 *      queue, so the next task in line becomes the head for the next tick.
 *   2. PendSV sets next_run_time = tick_count + 1 for non-periodic tasks,
 *      making the just-ran task ineligible until the next tick fires.
 * Together these ensure every task at the same priority level gets exactly
 * one time-slice per tick, in round-robin order.
 *
 * The os_rr_skip bitmap below was unused dead code and has been removed.
 */
/* Reset all scheduler-owned priority state. Kept here so os_init() can reset
   extension storage without exposing the static RR arrays as globals. */
extern "C" void _os_priority_queues_init(void) {
    os_ready_bitmap = 0;
    for (uint32_t i = 0; i < 32; ++i) os_pq_head[i] = nullptr;

#if OS_KERNEL_MAX_PRIORITIES > 32
    for (uint32_t i = 0; i < OS_PRIORITY_EXTRA_WORDS; ++i)
        _os_ready_bitmap_ext[i] = 0;
    for (uint32_t i = 0; i < OS_PRIORITY_EXTRA_COUNT; ++i)
        _os_pq_head_ext[i] = nullptr;
#endif
}

/* os_rr_skip, os_rr_is_skipped, os_rr_set_skip, os_rr_clear_all removed:
   round-robin fairness is handled by os_pq_rotate() + next_run_time gate.
   See comment above. */

/* os_ready_level_count removed — was unused dead code. */

extern "C" TCB* _os_pq_next(void) {
    const uint32_t now = tick_count;

    /* O(1) selection: use the bitmap to find the highest priority with
       an eligible task. For >32 priorities, _os_pq_highest_prio() scans
       extension words top-down using CLZ — O(1) bounded by word count. */
    uint8_t prio = _os_pq_highest_prio();
    while (prio > 0) {
        if (!_os_pq_bit_is_set(prio)) {
            prio--;
            continue;
        }
        TCB* head = *_os_pq_head_for(prio);
        if (!head) {
            _os_pq_clear_bit(prio);
            prio--;
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
        if (!selected) {
            prio--;
            continue;
        }

        if (selected != head) {
            prev->queue_next = selected->queue_next;
            selected->queue_next = head;
            *_os_pq_head_for(prio) = selected;
        }
        return selected;
    }
    return nullptr;
}

extern "C" void _os_pq_rotate(void) {
    const uint32_t now = tick_count;
    uint8_t p = 0xFF;

    /* Find the highest priority with an eligible task (O(1) bitmap scan) */
    uint8_t prio = _os_pq_highest_prio();
    while (prio > 0) {
        if (!_os_pq_bit_is_set(prio)) {
            prio--;
            continue;
        }
        for (TCB* t = *_os_pq_head_for(prio); t; t = t->queue_next) {
            if ((int32_t)(now - t->next_run_time) >= 0) {
                p = prio;
                break;
            }
        }
        if (p != 0xFF) break;
        prio--;
    }
    if (p == 0xFF) return;

    TCB* volatile* head_ptr = _os_pq_head_for(p);
    TCB* head = *head_ptr;
    if (!head) return;

    /* Single task at this priority — no rotation needed */
    if (!head->queue_next) return;

    /* Move head to tail for round-robin fairness */
    *head_ptr = head->queue_next;
    TCB* tail = *head_ptr;
    while (tail->queue_next) tail = tail->queue_next;
    tail->queue_next = head;
    head->queue_next = nullptr;
}

/* ═══════════════ Stack Init ═══════════════ */
void _os_stack_init(TCB* task) {
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
    ctx[2]  = (uint32_t)_os_task_exit;
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

void _os_reset_task_internal(TCB* task) {
    _os_stack_init(task);
    _os_pq_remove(task);
    task->state = TaskState::READY;
    task->delay_ticks = 0;
    task->blocking_on = nullptr;
    task->block_timeout = 0;
    task->wait_result = 0;
    task->next_run_time = tick_count;
    task->last_yield_tick = tick_count;
    task->priority = task->base_priority;
    task->mutex_nesting = 0;
    task->mutex_held_count = 0;
    _os_pq_add(task);
}

void _os_task_exit(void) {
    uint32_t cs = os_critical_enter();
    if (current_task) {
        _os_pq_remove(current_task); /* Remove from priority queue before state change */
        current_task->state = TaskState::INACTIVE;
    }
    os_critical_exit(cs);
    os_yield();
    while (1) { __asm volatile("nop"); }
}

void _os_wake_task(TCB* t, uint8_t result) {
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
    /* Every caller of _os_wake_task holds the PRIMASK critical section
       (os_tick, _os_event_signal[_from_isr], os_event_unregister,
       os_mutex_handoff, os_tickless_process), so a plain decrement is
       atomic with respect to any other context. ldrex/strex is not an
       option here: ARMv6-M (Cortex-M0/M0+) has no exclusive access
       instructions. */
    if (_os_blocked_count > 0) _os_blocked_count--;
    _os_pq_add(t);
}

bool _os_block_current(TaskState state, void* blocking_on,
                      uint32_t block_timeout, uint32_t delay_ticks) {
    if (!current_task) return false;
    _os_pq_remove(current_task);
    current_task->state = state;
    current_task->blocking_on = blocking_on;
    current_task->block_timeout = block_timeout;
    current_task->delay_ticks = delay_ticks;
    current_task->wait_result = 0;
    current_task->last_yield_tick = tick_count;
    /* All callers hold the PRIMASK critical section (os_delay_ms,
       os_event_wait, os_mutex_block_on), so a plain increment is atomic.
       ARMv6-M has no ldrex/strex. */
    _os_blocked_count++;
    return true;
}

uint32_t _os_ms_to_ticks(uint32_t ms) {
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
    if (_os_started) { _os_report_error(OSError::TASK_AFTER_START); return -1; }
    if (!task || !stack_mem || !entry) return -1;
    if (_os_find_task_by_entry(entry)) return -1;
    if (priority == 0) priority = 1;
    if (!_os_priority_valid(priority)) return -1;
    if (stack_size < 64) stack_size = 64;
    if (_os_task_count >= 255) { _os_report_error(OSError::TASK_AFTER_START); return -1; } /* ID is uint8_t */

    /* os_init() clears _os_task_count. Reset scheduler storage before the first
       task of a new lifecycle is inserted, so no stale >32-priority state
       can survive a reinitialization. */
    if (_os_task_count == 0) _os_priority_queues_init();

    task->id = _os_task_count++;
    task->name = name;
    task->entry = entry;
    task->priority = priority;
    task->base_priority = priority;
    task->mutex_nesting = 0;
    task->mutex_held_count = 0;

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
    _os_stack_init(task);
    task->next = _os_task_list;
    _os_task_list = task;
    _os_pq_add(task);
    return (int8_t)task->id;
}

TCB* _os_find_task_by_entry(void(*entry)(void)) {
    if (!entry) return nullptr;
    for (TCB* t = _os_task_list; t; t = t->next)
        if (t->entry == entry) return t;
    return nullptr;
}

TCB* _os_find_task_by_id(uint8_t id) {
    for (TCB* t = _os_task_list; t; t = t->next)
        if (t->id == id) return t;
    return nullptr;
}

/* Stop a task: revoke its eligibility to run without destroying it.
 *
 * Semantics (matching CMSIS-RTOS2 osThreadSuspend / FreeRTOS vTaskSuspend):
 *   - The TCB, stack and all scheduling state are preserved.
 *   - When os_task_start() is called the task continues from exactly the
 *     point where it stopped; its BLOCKED-wait state is preserved too, so
 *     a task suspended while waiting on an event resumes waiting on that
 *     same event rather than restarting from its entry point.
 *
 * Preconditions:
 *   - t must not be the idle task (the kernel needs idle to run).
 *   - A READY/RUNNING task is removed from the ready set and marked
 *     SUSPENDED; if it is the current task a context switch is forced so
 *     it stops immediately.  A BLOCKED task is marked SUSPENDED in place
 *     so the wakeup path (event signal / timeout) does not silently make
 *     it READY again while stopped. */
extern "C" void os_task_stop(void(*entry)(void)) {
    uint32_t cs = os_critical_enter();
    TCB* t = _os_find_task_by_entry(entry);
    if (t && t != &_os_idle_tcb) {
        TaskState st = t->state;
        if (st == TaskState::READY || st == TaskState::RUNNING) {
            _os_pq_remove(t);
            t->state = TaskState::SUSPENDED;
            if (t == current_task) OS_SCB_ICSR = OS_ICSR_PENDSVSET_Msk;
        } else if (st == TaskState::BLOCKED) {
            /* Abort the wait cleanly: clear blocking_on so the IPC object
               can be safely destroyed when the current scope exits.
               Decrement _os_blocked_count since we're removing it from the
               blocked set. Do NOT call _os_wake_task() here because that
               would enqueue the task as READY — we want SUSPENDED. */
            t->wait_result = 0;
            t->blocking_on = nullptr;
            t->block_timeout = 0;
            if (_os_blocked_count > 0) _os_blocked_count--;
            t->state = TaskState::SUSPENDED;
        }
    }
    os_critical_exit(cs);
}

/* Start (resume) a task previously stopped with os_task_stop().
 *
 * Semantics:
 *   - Re-queues the task at its current (possibly PI-boosted) priority and
 *     refreshes next_run_time so the period gate accepts it at once.
 *   - If the task was stopped while BLOCKED, it returns to BLOCKED and keeps
 *     waiting on the same object with its remaining timeout rather than
 *     being made READY prematurely.
 *   - A no-op for tasks that are not SUSPENDED, so start() cannot
 *     double-queue a task.  A task that was never stopped is untouched.
 *   - Also revives a task whose entry function returned / that was left
 *     INACTIVE, by re-initialising its stack (first start or restart). */
extern "C" void os_task_start(void(*entry)(void)) {
    uint32_t cs = os_critical_enter();
    TCB* t = _os_find_task_by_entry(entry);
    if (t && t != &_os_idle_tcb) {
        if (t->state == TaskState::SUSPENDED) {
            /* After H3 fix, os_task_stop always clears blocking_on, so a
               SUSPENDED task is always clean.  Reset mutex_nesting and
               priority to prevent stale PI state from a previous
               lock/unlock cycle leaking into the next use. */
            t->state = TaskState::READY;
            t->delay_ticks = 0;
            t->blocking_on = nullptr;
            t->block_timeout = 0;
            t->wait_result = 0;
            t->next_run_time = tick_count;
            t->last_yield_tick = tick_count;
            t->priority = t->base_priority;
            t->mutex_nesting = 0;
            t->mutex_held_count = 0;
            _os_pq_add(t);
        } else if (t->state == TaskState::INACTIVE) {
            /* First start, or restart after the entry function returned /
               the task was killed.  Reset stack + scheduling state. */
            _os_reset_task_internal(t);
            t->wdg_retries = 0;
        }
        OS_SCB_ICSR = OS_ICSR_PENDSVSET_Msk;
    }
    os_critical_exit(cs);
}

extern "C" uint8_t os_task_get_state(void(*entry)(void)) {
    uint32_t cs = os_critical_enter();
    TCB* t = _os_find_task_by_entry(entry);
    uint8_t result = t ? (uint8_t)t->state : 0xFF;
    os_critical_exit(cs);
    return result;
}

extern "C" bool os_task_isActive(void(*entry)(void)) {
    uint32_t cs = os_critical_enter();
    TCB* t = _os_find_task_by_entry(entry);
    uint8_t result = t ? (uint8_t)t->state : 0xFF;
    os_critical_exit(cs);
    /* A task is "active" only when it is READY or RUNNING.
       SUSPENDED (stopped), BLOCKED, and INACTIVE are all considered
       inactive — matching the CMSIS-RTOS2 osThreadGetState semantics
       where suspended/terminated threads are not "running". */
    return (result == (uint8_t)TaskState::READY ||
            result == (uint8_t)TaskState::RUNNING);
}

extern "C" uint8_t os_get_task_priority(void(*entry)(void)) {
    uint32_t cs = os_critical_enter();
    TCB* t = _os_find_task_by_entry(entry);
    uint8_t result = t ? t->priority : 0;
    os_critical_exit(cs);
    return result;
}

#if OS_KERNEL_TICKLESS_IDLE
bool _os_tickless_process(uint32_t skip) {
    tick_count += skip;
    bool woke = false;
    if (_os_blocked_count > 0) {
        for (TCB* task = _os_task_list; task; task = task->next) {
            if (task->state != TaskState::BLOCKED) continue;
            if (task->delay_ticks > 0) {
                if (task->delay_ticks <= skip) {
                    task->delay_ticks = 0;
                    _os_wake_on_delay_expiry(task);
                    /* Verify: after wake, next_run_time should be tick_count
                       (set by os_wake_on_delay_expiry in our fix). The period
                       gate in os_pq_next will then see (tick_count - next_run_time)
                       >= 0 and consider the task eligible. PendSV will override
                       next_run_time based on period_ticks when the task runs. */
                    if (task->period_ticks > 0) {
                        /* Periodic task: next_run_time should be current tick_count
                           so it's immediately eligible. PendSV will set it to
                           tick_count + period_ticks when the task runs.
                           os_wake_on_delay_expiry() already sets next_run_time =
                           tick_count; this is a defensive guard against any future
                           code path that might skip that assignment. */
                        if (task->next_run_time > tick_count) {
                            /* Defensive correction: force eligibility so the task
                               does not miss its release window after tickless idle.
                               Assert: this path should never execute in correct code.
                               If it fires, it indicates a regression in the wake path. */
                            task->next_run_time = tick_count;
                        }
                    }
                    woke = true;
                } else task->delay_ticks -= skip;
            } else if (task->block_timeout > 0) {
                if (task->block_timeout <= skip) {
                    task->block_timeout = 0;
                    _os_wake_on_timeout_expiry(task);
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
   cannot measure beyond one wrap period.  _os_us_accumulated holds the µs
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
    _os_time_fold();          /* lazy sample — same path os_tick uses */
    return _os_us_accumulated;
#else
    /* No DWT: scale the kernel tick (resolution = OS_KERNEL_TICK_PERIOD_US,
       wraps mod 2^32 µs like the ms counter above). */
    return (uint32_t)((uint64_t)tick_count * OS_KERNEL_TICK_PERIOD_US);
#endif
}

extern "C" uint32_t os_get_ms(void) {
    return tick_count / OS_TICKS_PER_MS;
}

extern "C" uint16_t os_get_task_count(void) { return _os_task_count; }
extern "C" uint32_t os_get_version(void) { return OS_VERSION_PACKED; }
extern "C" const char* os_get_version_string(void) { return OS_VERSION_STRING; }

/* ── Platform description ──
   These expose the values ZenOS_Config/Port derive automatically, so an
   application can report or assert on the detected target instead of
   hard-coding the MCU name.  os_get_capabilities() returns a bitmask of the
   compile-time capability detection documented in ZenOS_Port.hpp. */
extern "C" const char* os_get_family_name(void) { return OS_FAMILY_NAME; }
extern "C" const char* os_get_arch_name(void)   { return OS_ARCH_NAME; }

extern "C" uint32_t os_get_capabilities(void) {
    uint32_t caps = 0;
#if OS_HAS_MPU_HW
    caps |= OS_CAP_MPU;
#endif
#if OS_HAS_FPU_HW
    caps |= OS_CAP_FPU;
#endif
#if OS_HAS_USER_MPU_ACTIVE
    caps |= OS_CAP_MPU_ACTIVE;
#endif
#if OS_HAS_CRC_HW
    caps |= OS_CAP_CRC_HW;
#endif
#if OS_HAS_IWDG_HW
    caps |= OS_CAP_IWDG_HW;
#endif
#if OS_HAS_CYCLE_COUNTER
    caps |= OS_CAP_CYCCNT;
#endif
#if OS_HAS_VTOR
    caps |= OS_CAP_VTOR;
#endif
#if OS_KERNEL_TICKLESS_IDLE
    caps |= OS_CAP_TICKLESS;
#endif
    return caps;
}
