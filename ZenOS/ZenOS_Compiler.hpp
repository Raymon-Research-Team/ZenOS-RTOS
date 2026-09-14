#pragma once
/**
 * @file    ZenOS_Compiler.hpp
 * @brief   ZenOS RTOS — Compiler Abstraction Layer
 *
 * Provides portable intrinsics for GCC, Clang, IAR ARM, and Keil ARMCC/ARMCLANG.
 * All compiler-specific extensions are isolated here — no other ZenOS file
 * should use vendor-specific builtins directly.
 *
 * @author  Rahman Heidari <rahman.h22@gmail.com> — Raymon Research Team
 * @version 1.1.0
 */

#include <stdint.h>

/* ═══════════════ Compiler Detection ═══════════════ */

#if defined(__GNUC__) && !defined(__clang__)
    #define OS_COMPILER_GCC    1
    #define OS_COMPILER_CLANG  0
    #define OS_COMPILER_IAR    0
    #define OS_COMPILER_ARMCC  0
#elif defined(__clang__)
    #define OS_COMPILER_GCC    0
    #define OS_COMPILER_CLANG  1
    #define OS_COMPILER_IAR    0
    #define OS_COMPILER_ARMCC  0
#elif defined(__ICCARM__)
    #define OS_COMPILER_GCC    0
    #define OS_COMPILER_CLANG  0
    #define OS_COMPILER_IAR    1
    #define OS_COMPILER_ARMCC  0
#elif defined(__ARMCC_VERSION)
    #define OS_COMPILER_GCC    0
    #define OS_COMPILER_CLANG  0
    #define OS_COMPILER_IAR    0
    #define OS_COMPILER_ARMCC  1
#else
    #define OS_COMPILER_GCC    1  /* Assume GCC-compatible */
    #define OS_COMPILER_CLANG  0
    #define OS_COMPILER_IAR    0
    #define OS_COMPILER_ARMCC  0
#endif

/* ═══════════════ CLZ / CTZ (Count Leading/Trailing Zeros) ═══════════════ */

#if OS_COMPILER_GCC || OS_COMPILER_CLANG
    #define OS_CLZ(x)  __builtin_clz(x)
    #define OS_CTZ(x)  __builtin_ctz(x)
#elif OS_COMPILER_IAR
    #include <intrinsics.h>
    #define OS_CLZ(x)  __CLZ(x)
    #define OS_CTZ(x)  __iar_ctz(x)
#elif OS_COMPILER_ARMCC
    #define OS_CLZ(x)  __clz(x)
    #define OS_CTZ(x)  __rbit(x)  /* ARMCC has no direct CTZ; use rbit+clz */
#endif

/* ═══════════════ Memory Barriers ═══════════════ */

#if OS_COMPILER_GCC || OS_COMPILER_CLANG
    #define OS_DSB()  __asm volatile("dsb" ::: "memory")
    #define OS_ISB()  __asm volatile("isb" ::: "memory")
    #define OS_DMB()  __asm volatile("dmb" ::: "memory")
#elif OS_COMPILER_IAR
    #define OS_DSB()  __DSB()
    #define OS_ISB()  __ISB()
    #define OS_DMB()  __DMB()
#elif OS_COMPILER_ARMCC
    #define OS_DSB()  __dsb(0)
    #define OS_ISB()  __isb(0)
    #define OS_DMB()  __dmb(0)
#endif

/* ═══════════════ Interrupt Control ═══════════════ */

#if OS_COMPILER_GCC || OS_COMPILER_CLANG
    #define OS_DISABLE_IRQ()  __asm volatile("cpsid i" ::: "memory")
    #define OS_ENABLE_IRQ()   __asm volatile("cpsie i" ::: "memory")
    #define OS_GET_PRIMASK()  ({ uint32_t _p; __asm volatile("mrs %0, PRIMASK" : "=r"(_p)); _p; })
    #define OS_SET_PRIMASK(x) __asm volatile("msr PRIMASK, %0" :: "r"(x) : "memory")
#elif OS_COMPILER_IAR
    #define OS_DISABLE_IRQ()  __disable_interrupt()
    #define OS_ENABLE_IRQ()   __enable_interrupt()
    #define OS_GET_PRIMASK()  __get_PRIMASK()
    #define OS_SET_PRIMASK(x) __set_PRIMASK(x)
#elif OS_COMPILER_ARMCC
    #define OS_DISABLE_IRQ()  __disable_irq()
    #define OS_ENABLE_IRQ()   __enable_irq()
    #define OS_GET_PRIMASK()  __get_PRIMASK()
    #define OS_SET_PRIMASK(x) __set_PRIMASK(x)
#endif

/* ═══════════════ Function Attributes ═══════════════ */

#if OS_COMPILER_GCC || OS_COMPILER_CLANG
    #define OS_NAKED       __attribute__((naked))
    #define OS_USED        __attribute__((used))
    #define OS_WEAK        __attribute__((weak))
    #define OS_ALIGNED(n)  __attribute__((aligned(n)))
    #define OS_SECTION(s)  __attribute__((section(s)))
    #define OS_NOINLINE    __attribute__((noinline))
    #define OS_UNREACHABLE __builtin_unreachable()
#elif OS_COMPILER_IAR
    #define OS_NAKED       __stackless
    #define OS_USED        __root
    #define OS_WEAK        __weak
    #define OS_ALIGNED(n)  _Pragma(OS_STR_(OS_IAR_ALIGN_##n))
    #define OS_SECTION(s)  @ s
    #define OS_NOINLINE    __stackless
    #define OS_UNREACHABLE while(1)
#elif OS_COMPILER_ARMCC
    #define OS_NAKED       __attribute__((naked))
    #define OS_USED        __attribute__((used))
    #define OS_WEAK        __attribute__((weak))
    #define OS_ALIGNED(n)  __attribute__((aligned(n)))
    #define OS_SECTION(s)  __attribute__((section(s)))
    #define OS_NOINLINE    __attribute__((noinline))
    #define OS_UNREACHABLE __builtin_unreachable()
#endif

/* ═══════════════ Static Assert ═══════════════ */

#if defined(__cplusplus) && (__cplusplus >= 201103L)
    /* C++11+ has static_assert built-in */
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
    /* C11 has _Static_assert */
    #define static_assert _Static_assert
#else
    /* Fallback: negative-array-size trick */
    #define OS_CONCAT_(a, b) a##b
    #define OS_CONCAT(a, b) OS_CONCAT_(a, b)
    #define static_assert(cond, msg) \
        typedef char OS_CONCAT(static_assert_fail_, __LINE__)[(cond) ? 1 : -1]
#endif

/* ═══════════════ Volatile Access (prevent reordering) ═══════════════ */

/* Compiler-specific volatile barriers — ensure the compiler does not
   reorder or optimize away volatile accesses around these points. */
#if OS_COMPILER_GCC || OS_COMPILER_CLANG
    #define OS_COMPILER_BARRIER() __asm volatile("" ::: "memory")
#elif OS_COMPILER_IAR
    #define OS_COMPILER_BARRIER() __asm volatile("" ::: "memory")
#elif OS_COMPILER_ARMCC
    #define OS_COMPILER_BARRIER() __schedule_barrier()
#endif

/* ═══════════════ Byte Order ═══════════════ */

/* ARM Cortex-M is always little-endian */
#define OS_LITTLE_ENDIAN  1
#define OS_BIG_ENDIAN     0
