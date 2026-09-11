#define OS_BUILD
#include "ZenOS_Internal.hpp"

/* Correctness-first selector used by the new PendSV port. It does not depend
   on queue-head bookkeeping, so a blocked/period-gated highest-priority task
   can never hide an eligible lower-priority task. RAM cost is zero: selection
   walks the existing TCB list. For the usual small embedded task counts this
   is bounded and avoids the ~1 KB extended-priority table that the old design
   allocated for 256 priorities. */
extern "C" TCB* os_scheduler_select_next(void) {
    uint32_t cs = os_critical_enter();
    uint32_t now = tick_count;
    uint8_t best = 0;

    for (TCB* t = task_list; t; t = t->next) {
        if (t == &idle_tcb || t->state != TaskState::READY) continue;
        if (t->period_ticks && (int32_t)(now - t->next_run_time) < 0) continue;
        if (t->priority > best) best = t->priority;
    }

    if (best == 0U) {
        os_critical_exit(cs);
        return &idle_tcb;
    }

    /* Round-robin among equal-priority READY tasks, starting immediately after
       the current task and wrapping once through the existing task list. */
    TCB* current = current_task;
    if (current && current->priority == best) {
        bool after = false;
        for (TCB* t = task_list; t; t = t->next) {
            if (t == current) { after = true; continue; }
            if (after && t != &idle_tcb && t->state == TaskState::READY &&
                t->priority == best &&
                (!t->period_ticks || (int32_t)(now - t->next_run_time) >= 0)) {
                os_critical_exit(cs);
                return t;
            }
        }
        for (TCB* t = task_list; t && t != current; t = t->next) {
            if (t != &idle_tcb && t->state == TaskState::READY &&
                t->priority == best &&
                (!t->period_ticks || (int32_t)(now - t->next_run_time) >= 0)) {
                os_critical_exit(cs);
                return t;
            }
        }
    }

    for (TCB* t = task_list; t; t = t->next) {
        if (t != &idle_tcb && t->state == TaskState::READY && t->priority == best &&
            (!t->period_ticks || (int32_t)(now - t->next_run_time) >= 0)) {
            os_critical_exit(cs);
            return t;
        }
    }

    os_critical_exit(cs);
    return &idle_tcb;
}
