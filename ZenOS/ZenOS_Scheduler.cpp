#define OS_BUILD
#include "ZenOS_Internal.hpp"

/*
 * Scheduler design:
 *   - priorities 1..31 keep the original 32 queue heads for fast O(1) common
 *     scheduling and zero additional RAM in the default configuration;
 *   - priorities 32..255 use only a bitmap plus the existing TCB task list.
 *     This avoids allocating 224 queue-head pointers (~896 bytes) merely to
 *     support a 256-priority build. High-priority selection is O(number of
 *     tasks), while the common <=31 path remains O(1).
 *
 * All scheduler mutation is performed with interrupts masked. This is a
 * single-core RTOS; LDREX/STREX is therefore unnecessary for scheduler
 * counters and would make the kernel dependent on optional ISA features.
 */

#if OS_KERNEL_MAX_PRIORITIES > 32
static volatile uint32_t os_ready_bitmap_ext[OS_PRIORITY_EXTRA_WORDS] = {0};
#endif
static TCB* os_high_rr_cursor = nullptr;

static inline bool os_priority_valid(uint8_t p) {
    return p > 0U && p < OS_KERNEL_MAX_PRIORITIES;
}

static inline bool os_high_priority(uint8_t p) { return p >= 32U; }

static inline TCB* volatile* os_pq_head_for(uint8_t p) {
    if (p >= 32U) return nullptr;
    return &os_pq_head[p];
}

static inline void os_pq_set_bit(uint8_t p) {
    if (!os_priority_valid(p)) return;
    if (p < 32U) {
        os_ready_bitmap |= (1UL << p);
    } else {
#if OS_KERNEL_MAX_PRIORITIES > 32
        uint8_t q = (uint8_t)(p - 32U);
        os_ready_bitmap_ext[q >> 5] |= (1UL << (q & 31U));
#endif
    }
}

static inline void os_pq_clear_bit(uint8_t p) {
    if (!os_priority_valid(p)) return;
    if (p < 32U) {
        os_ready_bitmap &= ~(1UL << p);
    } else {
#if OS_KERNEL_MAX_PRIORITIES > 32
        uint8_t q = (uint8_t)(p - 32U);
        os_ready_bitmap_ext[q >> 5] &= ~(1UL << (q & 31U));
#endif
    }
}

static inline bool os_pq_bit_is_set(uint8_t p) {
    if (!os_priority_valid(p)) return false;
    if (p < 32U) return (os_ready_bitmap & (1UL << p)) != 0U;
#if OS_KERNEL_MAX_PRIORITIES > 32
    uint8_t q = (uint8_t)(p - 32U);
    return (os_ready_bitmap_ext[q >> 5] & (1UL << (q & 31U))) != 0U;
#else
    return false;
#endif
}

static uint8_t os_pq_highest_prio(void) {
#if OS_KERNEL_MAX_PRIORITIES > 32
    for (int word = (int)OS_PRIORITY_EXTRA_WORDS - 1; word >= 0; --word) {
        uint32_t bits = os_ready_bitmap_ext[word];
        if (bits != 0U) {
            uint8_t bit = (uint8_t)(31U - (uint32_t)__builtin_clz(bits));
            uint16_t p = (uint16_t)(32U + (uint16_t)word * 32U + bit);
            if (p < OS_KERNEL_MAX_PRIORITIES) return (uint8_t)p;
        }
    }
#endif
    if (os_ready_bitmap != 0U)
        return (uint8_t)(31U - (uint32_t)__builtin_clz(os_ready_bitmap));
    return 0U;
}

void os_pq_add(TCB* task) {
    if (!task || task == &idle_tcb || !os_priority_valid(task->priority)) return;
    uint8_t p = task->priority;
    if (p < 32U) {
        TCB* volatile* head = os_pq_head_for(p);
        task->queue_next = *head;
        *head = task;
    } else {
        /* High priorities intentionally use the task list instead of 224
           persistent queue-head pointers. */
        task->queue_next = nullptr;
    }
    os_pq_set_bit(p);
}

void os_pq_remove(TCB* task) {
    if (!task || !os_priority_valid(task->priority)) return;
    uint8_t p = task->priority;
    if (p >= 32U) {
        /* There is no per-priority linked queue in the extended range. */
        bool any = false;
        for (TCB* t = task_list; t; t = t->next) {
            if (t != task && t != &idle_tcb && t->state == TaskState::READY &&
                t->priority == p) { any = true; break; }
        }
        if (!any) os_pq_clear_bit(p);
        task->queue_next = nullptr;
        return;
    }

    TCB* volatile* pp = os_pq_head_for(p);
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

extern "C" void os_priority_queues_init(void) {
    os_ready_bitmap = 0;
    for (uint32_t i = 0; i < 32U; ++i) os_pq_head[i] = nullptr;
#if OS_KERNEL_MAX_PRIORITIES > 32
    for (uint32_t i = 0; i < OS_PRIORITY_EXTRA_WORDS; ++i)
        os_ready_bitmap_ext[i] = 0;
#endif
    os_high_rr_cursor = nullptr;
}

static TCB* os_select_high_priority(uint8_t p, uint32_t now) {
    TCB* first = nullptr;
    TCB* after = nullptr;
    bool pass_cursor = (os_high_rr_cursor == nullptr);

    for (TCB* t = task_list; t; t = t->next) {
        if (t == &idle_tcb || t->state != TaskState::READY || t->priority != p)
            continue;
        if (t->period_ticks != 0U && (int32_t)(now - t->next_run_time) < 0)
            continue;
        if (!first) first = t;
        if (pass_cursor && !after) after = t;
        if (t == os_high_rr_cursor) pass_cursor = true;
    }
    if (after) return after;
    return first;
}

extern "C" TCB* os_pq_next(void) {
    const uint32_t now = tick_count;
    uint8_t highest = os_pq_highest_prio();
    if (highest == 0U) return nullptr;

    if (highest >= 32U) {
        TCB* selected = os_select_high_priority(highest, now);
        if (!selected) {
            os_pq_clear_bit(highest);
            return nullptr;
        }
        return selected;
    }

    TCB* head = *os_pq_head_for(highest);
    TCB* prev = nullptr;
    for (TCB* t = head; t; t = t->queue_next) {
        if (t->period_ticks == 0U || (int32_t)(now - t->next_run_time) >= 0) {
            if (t != head) {
                prev->queue_next = t->queue_next;
                t->queue_next = head;
                *os_pq_head_for(highest) = t;
            }
            return t;
        }
        prev = t;
    }
    return nullptr;
}

extern "C" void os_pq_rotate(void) {
    uint8_t p = os_pq_highest_prio();
    if (p == 0U) return;
    uint32_t now = tick_count;

    if (p >= 32U) {
        TCB* current = os_select_high_priority(p, now);
        if (current) os_high_rr_cursor = current;
        return;
    }

    TCB* head = *os_pq_head_for(p);
    if (!head || !head->queue_next) return;
    TCB* tail = head->queue_next;
    while (tail->queue_next) tail = tail->queue_next;
    *os_pq_head_for(p) = head->queue_next;
    head->queue_next = nullptr;
    tail->queue_next = head;
}

void os_stack_init(TCB* task) {
    uint32_t* base = task->stack_base;
    uint32_t size = task->stack_size;
    if (!base || size < 25U) return; /* 8 canary + 17 context words */

    for (uint32_t i = OS_STACK_CANARY_COUNT; i < size; ++i)
        base[i] = 0xA5A5A5A5UL;
    for (uint32_t i = 0; i < OS_STACK_CANARY_COUNT; ++i)
        base[i] = OS_STACK_CANARY;

    uint32_t* sp = base + size;
    sp = (uint32_t*)((uintptr_t)sp & ~(uintptr_t)7U);

    /* Hardware frame: xPSR, PC, LR, R12, R3, R2, R1, R0. */
    uint32_t hw[8] = {
        0x01000000UL, (uint32_t)task->entry, (uint32_t)os_task_exit,
        0x0000000CUL, 0x00000003UL, 0x00000002UL, 0x00000001UL, 0x00000000UL
    };
    /* Software frame: R4-R11 plus EXC_RETURN. The LR slot is essential on
       FPU targets because bit[4] distinguishes basic/extended FP frames. */
    uint32_t sw[9] = {
        0x00000004UL, 0x00000005UL, 0x00000006UL, 0x00000007UL,
        0x00000008UL, 0x00000009UL, 0x0000000AUL, 0x0000000BUL,
        0xFFFFFFFDUL
    };

    sp -= 8U;
    for (uint32_t i = 0; i < 8U; ++i) sp[i] = hw[i];
    sp -= 9U;
    for (uint32_t i = 0; i < 9U; ++i) sp[i] = sw[i];
    task->stack_top = sp;
}

void os_reset_task_internal(TCB* task) {
    if (!task) return;
    os_pq_remove(task);
    os_stack_init(task);
    task->state = TaskState::READY;
    task->delay_ticks = 0;
    task->blocking_on = nullptr;
    task->block_timeout = 0;
    task->wait_result = 0;
    task->next_run_time = tick_count;
    task->last_yield_tick = tick_count;
    task->priority = task->base_priority;
    task->mutex_nesting = 0;
    task->wdg_retries = 0;
    os_pq_add(task);
}

void os_task_exit(void) {
    uint32_t cs = os_critical_enter();
    if (current_task) current_task->state = TaskState::INACTIVE;
    os_critical_exit(cs);
    os_yield();
    for (;;) __asm volatile("nop");
}

void os_wake_task(TCB* t, uint8_t result) {
    if (!t) return;
    t->wait_result = result;
    t->state = TaskState::READY;
    t->blocking_on = nullptr;
    t->block_timeout = 0;
    t->last_yield_tick = tick_count;
    if (blocked_count != 0U) os_atomic_dec_saturating(&blocked_count);
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
    os_atomic_inc(&blocked_count);
    return true;
}

uint32_t os_ms_to_ticks(uint32_t ms) {
    if (ms == OS_WAIT_FOREVER) return 0U;
    if (OS_TICKS_PER_MS == 0U) return 0U;
    if (ms <= (0xFFFFFFFFUL / OS_TICKS_PER_MS)) {
        uint32_t r = ms * OS_TICKS_PER_MS;
        return r ? r : 1U;
    }
    return 0xFFFFFFFEUL;
}

extern "C" int8_t _os_task_create_internal(
    TCB* task, uint32_t* stack_mem, uint32_t stack_size,
    const char* name, void(*entry)(void), uint8_t priority, uint32_t period_ms)
{
    if (os_started) { os_report_error(OSError::TASK_AFTER_START); return -1; }
    if (!task || !stack_mem || !entry) return -1;
    if (os_find_task_by_entry(entry)) return -1;
    if (priority == 0U) priority = 1U;
    if (!os_priority_valid(priority)) return -1;
    if (stack_size < 25U) return -1;
    if (task_count == 0U) os_priority_queues_init();

    task->id = (uint8_t)task_count++;
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
    task->queue_next = nullptr;
#if OS_SMP_CORES > 1
    task->core_id = 0;
    task->_pad[0] = task->_pad[1] = task->_pad[2] = 0;
#endif
#if OS_MONITOR_ENABLED
    task->peak_sp = nullptr;
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
    addr = (addr + 31U) & ~(uintptr_t)31U;
#else
    addr = (addr + 7U) & ~(uintptr_t)7U;
#endif
    uintptr_t end = (uintptr_t)stack_mem + (uintptr_t)stack_size * sizeof(uint32_t);
    if (addr >= end) return -1;
    task->stack_base = (uint32_t*)addr;
    task->stack_size = (uint32_t)((end - addr) / sizeof(uint32_t));
    if (task->stack_size < 25U) return -1;
    os_stack_init(task);
#if OS_MONITOR_ENABLED
    task->peak_sp = task->stack_top;
#endif
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
        if (t->state == TaskState::BLOCKED && blocked_count) --blocked_count;
        t->state = TaskState::INACTIVE;
        t->delay_ticks = 0;
        t->blocking_on = nullptr;
        t->block_timeout = 0;
        if (t == current_task) {
            current_task = nullptr;
            OS_SCB_ICSR = OS_ICSR_PENDSVSET_Msk;
        }
    }
    os_critical_exit(cs);
}

extern "C" void os_task_start(void(*entry)(void)) {
    uint32_t cs = os_critical_enter();
    TCB* t = os_find_task_by_entry(entry);
    if (t && t->state == TaskState::INACTIVE) {
        os_reset_task_internal(t);
        OS_SCB_ICSR = OS_ICSR_PENDSVSET_Msk;
    }
    os_critical_exit(cs);
}

#if OS_MONITOR_DEADLINE || OS_MONITOR_TCB_INTEGRITY || OS_MONITOR_ERROR_LOG
extern "C" uint8_t os_task_get_state(void(*entry)(void)) {
    uint32_t cs = os_critical_enter();
    TCB* t = os_find_task_by_entry(entry);
    uint8_t r = t ? (uint8_t)t->state : 0xFFU;
    os_critical_exit(cs);
    return r;
}
#endif

extern "C" bool os_task_isActive(void(*entry)(void)) {
    if (!entry) return false;
    uint32_t cs = os_critical_enter();
    TCB* t = os_find_task_by_entry(entry);
    bool r = t && t->state != TaskState::INACTIVE;
    os_critical_exit(cs);
    return r;
}

extern "C" uint8_t os_get_task_priority(void(*entry)(void)) {
    uint32_t cs = os_critical_enter();
    TCB* t = os_find_task_by_entry(entry);
    uint8_t r = t ? t->priority : 0U;
    os_critical_exit(cs);
    return r;
}

/* Architecture-independent scheduler entry points used by the PendSV port. */
extern "C" TCB* os_pendsv_select_task(void) {
    uint32_t cs = os_critical_enter();
    TCB* next = os_pq_next();
    if (!next) next = &idle_tcb;
    os_critical_exit(cs);
    return next;
}

extern "C" void os_pendsv_activate_task(TCB* task) {
    if (!task) task = &idle_tcb;
    uint32_t cs = os_critical_enter();
    current_task = task;
    task->state = TaskState::RUNNING;
    task->last_yield_tick = tick_count;
    os_critical_exit(cs);
}

#if OS_TOOL_TICKLESS_IDLE
bool os_tickless_process(uint32_t skip) {
    tick_count += skip;
    bool woke = false;
    if (blocked_count == 0U) return false;
    for (TCB* task = task_list; task; task = task->next) {
        if (task->state != TaskState::BLOCKED) continue;
        if (task->delay_ticks > 0U) {
            if (task->delay_ticks <= skip) {
                task->delay_ticks = 0;
                os_wake_on_delay_expiry(task);
                woke = true;
            } else task->delay_ticks -= skip;
        } else if (task->block_timeout > 0U) {
            if (task->block_timeout <= skip) {
                task->block_timeout = 0;
                os_wake_on_timeout_expiry(task);
                woke = true;
            } else task->block_timeout -= skip;
        }
    }
    return woke;
}
#endif

extern "C" uint32_t os_get_tick(void) { return tick_count; }
extern "C" uint32_t os_get_us(void) {
#if OS_HAS_CYCLE_COUNTER
    os_time_fold();
    return os_us_accumulated;
#else
    return (uint32_t)((uint64_t)tick_count * OS_KERNEL_TICK_PERIOD_US);
#endif
}
extern "C" uint32_t os_get_ms(void) { return tick_count / OS_TICKS_PER_MS; }
extern "C" uint16_t os_get_task_count(void) { return task_count; }
extern "C" uint32_t os_get_version(void) { return OS_VERSION_PACKED; }
extern "C" const char* os_get_version_string(void) { return OS_VERSION_STRING; }
