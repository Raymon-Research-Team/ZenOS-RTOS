/**
 * @file    ZenOS_Scheduler.cpp
 * @brief   ZenOS RTOS — Configurable priority bitmap scheduler & task management
 *
 * Extracted from ZenOS.cpp as part of the modular split.
 * Priority slots are configured by OS_MAX_PRIORITIES (2..256), with
 * priority 0 reserved and usable priorities 1..OS_MAX_PRIORITIES-1.
 *
 * @author  Rahman Heidari <rahman.h22@gmail.com> — Raymon Research Team
 * @version 1.0.0
 */

#define OS_BUILD
#include "ZenOS_Internal.hpp"

#if OS_MAX_PRIORITIES > 32
volatile uint32_t os_ready_bitmap_ext[OS_PRIORITY_EXTRA_WORDS] = {0};
TCB* volatile os_pq_head_ext[OS_PRIORITY_EXTRA_COUNT] = {nullptr};
#endif

static inline bool os_priority_valid(uint8_t prio) {
    return prio > 0 && prio < OS_MAX_PRIORITIES;
}

static inline TCB* volatile* os_pq_head_for(uint8_t prio) {
    if (prio < 32) return &os_pq_head[prio];
#if OS_MAX_PRIORITIES > 32
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
#if OS_MAX_PRIORITIES > 32
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
#if OS_MAX_PRIORITIES > 32
    uint8_t p = (uint8_t)(prio - 32);
    os_ready_bitmap_ext[p >> 5] &= ~(1UL << (p & 31));
#endif
}

static inline bool os_pq_bit_is_set(uint8_t prio) {
    if (!os_priority_valid(prio)) return false;
    if (prio < 32) return (os_ready_bitmap & (1UL << prio)) != 0;
#if OS_MAX_PRIORITIES > 32
    uint8_t p = (uint8_t)(prio - 32);
    return (os_ready_bitmap_ext[p >> 5] & (1UL << (p & 31))) != 0;
#else
    return false;
#endif
}

static inline uint8_t os_pq_highest_prio(void) {
#if OS_MAX_PRIORITIES > 32
    for (int word = (int)OS_PRIORITY_EXTRA_WORDS - 1; word >= 0; --word) {
        uint32_t bits = os_ready_bitmap_ext[word];
        if (bits != 0) {
            uint8_t bit = (uint8_t)(31UL - (uint32_t)__builtin_clz(bits));
            uint16_t prio = (uint16_t)(32U + (uint16_t)word * 32U + bit);
            if (prio < OS_MAX_PRIORITIES) return (uint8_t)prio;
        }
    }
#endif
    if (os_ready_bitmap != 0)
        return (uint8_t)(31UL - (uint32_t)__builtin_clz(os_ready_bitmap));
    return 0;
}

/* ── Round-Robin Throttle ── */
#if OS_MAX_PRIORITIES <= 32
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

#if OS_MAX_PRIORITIES > 32
    for (uint32_t i = 0; i < OS_PRIORITY_EXTRA_WORDS; ++i) {
        os_ready_bitmap_ext[i] = 0;
        os_rr_skip_ext[i] = 0;
    }
    for (uint32_t i = 0; i < OS_PRIORITY_EXTRA_COUNT; ++i)
        os_pq_head_ext[i] = nullptr;
#endif

    os_rr_skip = 0;
    os_rr_skip_quota = 0;
}

static inline bool os_rr_is_skipped(uint8_t prio) {
    if (!os_priority_valid(prio)) return false;
    if (prio < 32) return (os_rr_skip & (1UL << prio)) != 0;
#if OS_MAX_PRIORITIES > 32
    uint8_t p = (uint8_t)(prio - 32);
    return (os_rr_skip_ext[p >> 5] & (1UL << (p & 31))) != 0;
#else
    return false;
#endif
}

/* ═══════════════ Priority Queue Operations ═══════════════ */
void os_pq_add(TCB* task) {
    if (!task || !os_priority_valid(task->priority)) return;
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

