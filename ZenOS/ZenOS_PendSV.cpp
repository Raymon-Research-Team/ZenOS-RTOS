#define OS_BUILD
#include "ZenOS_Internal.hpp"
#undef OS_PendSV_Handler

extern "C" OS_NAKED OS_USED void OS_PendSV_Handler(void) {
    __asm volatile(
        "mrs r0, psp                         \n"
        "isb                                \n"
#if OS_HAS_FPU
        "tst lr, #0x10                       \n"
        "it eq                               \n"
        "vstmdbeq r0!, {s16-s31}              \n"
        "stmdb r0!, {r4-r11, lr}              \n"
#else
        "stmdb r0!, {r4-r11}                 \n"
#endif
        "ldr r1, =current_task                \n"
        "ldr r2, [r1]                         \n"
        "cmp r2, #0                            \n"
        "beq 1f                               \n"
        "str r0, [r2, #" OS_STR(OS_OFF_STACK_TOP) "]\n"
        /* A voluntary/preemptive switch leaves RUNNING -> READY. A task that
           explicitly blocked/stopped itself has already changed state and
           must remain BLOCKED/INACTIVE. The existing queue node is retained,
           so changing only the state preserves O(1) queue membership. */
        "ldrb r3, [r2, #" OS_STR(OS_OFF_STATE) "]\n"
        "cmp r3, #2                            \n"
        "bne 1f                               \n"
        "movs r3, #1                           \n"
        "strb r3, [r2, #" OS_STR(OS_OFF_STATE) "]\n"
        "1:                                   \n"
        "push {r1, r2, r3, lr}                \n"
        "bl os_pendsv_select_task              \n"
        "mov r4, r0                            \n"
        "mov r0, r4                            \n"
        "bl os_pendsv_activate_task            \n"
        "ldr r1, =tick_count                   \n"
        "ldr r2, [r1]                          \n"
        "ldr r3, [r4, #" OS_STR(OS_OFF_PERIOD_TICKS) "]\n"
        "cmp r3, #0                            \n"
        "beq 2f                               \n"
        "adds r2, r2, r3                       \n"
        "b 3f                                \n"
        "2:                                   \n"
        "adds r2, r2, #1                       \n"
        "3:                                   \n"
        "str r2, [r4, #" OS_STR(OS_OFF_NEXT_RUN_TIME) "]\n"
#if OS_SAFETY_MPU
        "mov r0, r4                            \n"
        "bl os_mpu_configure_task              \n"
#endif
        "pop {r1, r2, r3, lr}                 \n"
        "ldr r0, [r4, #" OS_STR(OS_OFF_STACK_TOP) "]\n"
#if OS_HAS_FPU
        "ldmia r0!, {r4-r11}                  \n"
        "ldr r3, [r0, #0]                     \n"
        "lsrs r3, r3, #28                     \n"
        "cmp r3, #0xF                         \n"
        "bne 4f                               \n"
        "ldr lr, [r0, #0]                     \n"
        "adds r0, r0, #4                      \n"
        "tst lr, #0x10                        \n"
        "it eq                                \n"
        "vldmiaeq r0!, {s16-s31}              \n"
        "b 5f                                \n"
        "4:                                   \n"
        "ldr lr, =0xFFFFFFFD                 \n"
        "5:                                   \n"
#else
        "ldmia r0!, {r4-r11}                  \n"
#endif
        "msr psp, r0                          \n"
        "isb                                  \n"
        "bx lr                                \n"
        ".ltorg                               \n"
    );
}
