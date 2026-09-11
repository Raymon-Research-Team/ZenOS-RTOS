#define OS_BUILD
#include "ZenOS.hpp"
#include <cstddef>

/* Assembly uses these byte offsets. Keep the contract machine-checked: a TCB
   field insertion/reordering must fail the build instead of silently corrupting
   scheduler state. */
static_assert(offsetof(TCB, state) == OS_OFF_STATE, "TCB state offset mismatch");
static_assert(offsetof(TCB, period_ticks) == OS_OFF_PERIOD_TICKS, "TCB period offset mismatch");
static_assert(offsetof(TCB, next_run_time) == OS_OFF_NEXT_RUN_TIME, "TCB next_run_time offset mismatch");
static_assert(offsetof(TCB, stack_top) == OS_OFF_STACK_TOP, "TCB stack_top offset mismatch");
