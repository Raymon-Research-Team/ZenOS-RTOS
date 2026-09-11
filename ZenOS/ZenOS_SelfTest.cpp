#define OS_BUILD
#include "ZenOS_Internal.hpp"
#include "ZenOS_SelfTest.hpp"

static uint32_t os_self_test_task_list(void) {
    uint32_t errors = 0;
    uint16_t count = 0;
    for (TCB* t = task_list; t; t = t->next) {
        if (t == &idle_tcb) continue;
        ++count;
        if (!t->entry || !t->stack_base || t->stack_size < 24U) errors |= 1UL << 1;
        if (t->priority == 0U || t->priority >= OS_KERNEL_MAX_PRIORITIES) errors |= 1UL << 1;
#if OS_MONITOR_TCB_INTEGRITY
        if (t->magic != OS_TCB_MAGIC) errors |= 1UL << 1;
#endif
        if (t->stack_base && t->stack_size >= OS_STACK_CANARY_COUNT) {
            uintptr_t lo = (uintptr_t)t->stack_base;
            uintptr_t hi = lo + (uintptr_t)t->stack_size * sizeof(uint32_t);
            uintptr_t sp = (uintptr_t)t->stack_top;
            if (sp < lo + (uintptr_t)OS_STACK_CANARY_COUNT * sizeof(uint32_t) || sp > hi) {
                errors |= 1UL << 2;
            }
            for (uint32_t i = 0; i < OS_STACK_CANARY_COUNT; ++i) {
                if (t->stack_base[i] != OS_STACK_CANARY) {
                    errors |= 1UL << 2;
                    break;
                }
            }
        } else {
            errors |= 1UL << 2;
        }
    }
    if (count != task_count) errors |= 1UL << 0;
    return errors;
}

static uint32_t os_self_test_queues(void) {
    uint32_t errors = 0;

    for (uint32_t p = 1; p < 32U && p < OS_KERNEL_MAX_PRIORITIES; ++p) {
        uint32_t queued = 0;
        for (TCB* q = os_pq_head[p]; q; q = q->queue_next) {
            ++queued;
            if (queued > task_count + 1U) return errors | (1UL << 3);
            if (q->state != TaskState::READY || q->priority != p) errors |= 1UL << 3;
        }

        uint32_t ready = 0;
        for (TCB* t = task_list; t; t = t->next) {
            if (t == &idle_tcb) continue;
            if (t->state == TaskState::READY && t->priority == p) ++ready;
        }
        if (queued != ready) errors |= 1UL << 3;
        bool bit = (os_ready_bitmap & (1UL << p)) != 0U;
        if (bit != (ready != 0U)) errors |= 1UL << 3;
    }

    for (TCB* t = task_list; t; t = t->next) {
        if (t == &idle_tcb || t->priority < 32U) continue;
        if (t->state == TaskState::READY && t->queue_next != nullptr) errors |= 1UL << 3;
    }

    return errors;
}

extern "C" uint32_t os_self_test(void) {
    uint32_t cs = os_critical_enter();
    uint32_t result = 0;
    result |= os_self_test_task_list();
    result |= os_self_test_queues();

    if (current_task && current_task != &idle_tcb) {
        bool found = false;
        for (TCB* t = task_list; t; t = t->next) {
            if (t == current_task) { found = true; break; }
        }
        if (!found || current_task->state != TaskState::RUNNING) result |= 1UL << 4;
    }

    os_critical_exit(cs);
    return result;
}
