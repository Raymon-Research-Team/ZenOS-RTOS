#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Runtime kernel invariant check. No heap, no extra persistent RAM.
 * Return value is a bit mask; zero means all requested checks passed.
 * Bit 0: task-list/count consistency
 * Bit 1: TCB fields and priorities are valid
 * Bit 2: stack bounds/canaries are valid
 * Bit 3: READY/queue state is consistent
 * Bit 4: current_task is valid (or scheduler has no current task yet)
 */
uint32_t os_self_test(void);

#ifdef __cplusplus
}
#endif
