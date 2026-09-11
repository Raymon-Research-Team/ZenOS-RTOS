#define OS_BUILD
#include "ZenOS_Internal.hpp"

/* The legacy definition in ZenOS.cpp is renamed by ZenOS_Port.hpp for internal
   translation units. This file is the sole exported PendSV implementation. */
#undef OS_PendSV_Handler

extern "C" OS_NAKED OS_USED void OS_PendSV_Handler(void) {
    __asm volatile(
        "mrs r0, psp                         \n"
        "isb                                \n"
#if OS_HAS_FPU
        /* Lazy FP stacking makes LR[4] the authoritative indication of
           whether the hardware created an extended frame. S16-S31 are the
           callee-saved FP registers and must be carried by the RTOS. */
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
        "str r0, [r2, #0]                     \n"
        "1:                                   \n"
        /* Keep the selector and current-task pointer safe across C calls.
           Four pushed registers preserve the required 8-byte MSP alignment. */
        "push {r1, r2, r3, lr}                \n"
        "bl os_pendsv_select_task              \n"
        "mov r4, r0                            \n"
        "mov r0, r4                            \n"
        "bl os_pendsv_activate_task            \n"
#if OS_SAFETY_MPU
        "mov r0, r4                            \n"
        "bl os_mpu_configure_task              \n"
#endif
        "pop {r1, r2, r3, lr}                 \n"
        "ldr r0, [r4, #0]                     \n"
#if OS_HAS_FPU
        /* Normal saved-FP context contains EXC_RETURN immediately after
           R4-R11. A never-run task still has the original 16-word frame from
           os_stack_init(), so its next word is xPSR rather than EXC_RETURN. */
        "ldmia r0!, {r4-r11}                  \n"
        "ldr r3, [r0, #0]                     \n"
        "lsrs r3, r3, #28                     \n"
        "cmp r3, #0xF                         \n"
        "bne 2f                               \n"
        "ldr lr, [r0, #0]                     \n"
        "adds r0, r0, #4                     \n"
        "tst lr, #0x10                       \n"
        "it eq                               \n"
        "vldmiaeq r0!, {s16-s31}              \n"
        "b 3f                                \n"
        "2:                                   \n"
        "ldr lr, =0xFFFFFFFD                 \n"
        "3:                                   \n"
#else
        "ldmia r0!, {r4-r11}                  \n"
#endif
        "msr psp, r0                          \n"
        "isb                                  \n"
        "bx lr                                \n"
    );
}
