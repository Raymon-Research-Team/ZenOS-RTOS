#pragma once
/**
 * @file    ZenOS_Debug.hpp
 * @brief   ZenOS RTOS — Debug UART Subsystem
 *
 * A lightweight, compile-time switchable UART debug channel that provides
 * live runtime tracing for context switches, errors, faults, and periodic
 * statistics.  When OS_DEBUG_UART is 0, EVERY debug symbol is compiled
 * away — zero code/RAM overhead.
 *
 * Features:
 *   - Context switch tracing (task name/ID before→after)
 *   - Error reporting with code, severity, current task
 *   - HardFault dump (full ZenOS_FaultContext)
 *   - Periodic stats: CPU usage, stack watermarks, deadline misses
 *   - Boot banner: OS_FAMILY_NAME, version, clock frequency
 *   - Lightweight printf-like formatting (no stdio, no heap)
 *   - Compile-time log level filtering (ERROR/WARN/INFO/TRACE)
 *   - Optional interrupt-driven ring buffer (vs polling)
 *
 * SAFETY: This is a debug-only channel.  If UART hardware is unavailable
 * or the TX line is disconnected, the kernel MUST NOT block or crash.
 * All debug output is fire-and-forget.
 *
 * @author  Rahman Heidari <rahman.h22@gmail.com> — Raymon Research Team
 * @version 1.1.0
 */

#include <stdint.h>
#include <stddef.h>

/* ═══════════════ Compile-Time Switch ═══════════════
 * Set to 1 in ZenOS_Config.hpp to enable the debug UART.
 * When 0, ALL debug code is #if'd away — zero overhead.
 */
#ifndef OS_DEBUG_UART
#define OS_DEBUG_UART 0
#endif

#if OS_DEBUG_UART

/* ═══════════════ Backend Selection ═══════════════
 * OS_DEBUG_BACKEND selects the output channel:
 *   0 = UART (default, requires physical TX pin)
 *   1 = RTT   (SEGGER RTT-compatible, no pin needed, SWD only)
 *
 * Override in project defines: -DOS_DEBUG_BACKEND=1
 */
#ifndef OS_DEBUG_BACKEND
#define OS_DEBUG_BACKEND  0   /* UART */
#endif

#define OS_DEBUG_BACKEND_UART  0
#define OS_DEBUG_BACKEND_RTT   1

/* ═══════════════ User Configuration (UART backend only) ═══════════════
 * Override these in your project defines (-D) or ZenOS_Config.hpp.
 * Only used when OS_DEBUG_BACKEND == OS_DEBUG_BACKEND_UART.
 */
#if (OS_DEBUG_BACKEND == OS_DEBUG_BACKEND_UART)

/* Which USART to use (default: USART1 at PA9/PA10 for STM32F1) */
#ifndef OS_DEBUG_USART_BASE
#define OS_DEBUG_USART_BASE   0x40013800UL   /* USART1 base address */
#endif

/* USART register offsets (same for USART1/2/3 on STM32F1) */
#ifndef OS_DEBUG_USART_SR
#define OS_DEBUG_USART_SR     (*(volatile uint32_t*)(OS_DEBUG_USART_BASE + 0x00UL))
#endif
#ifndef OS_DEBUG_USART_DR
#define OS_DEBUG_USART_DR     (*(volatile uint32_t*)(OS_DEBUG_USART_BASE + 0x04UL))
#endif
#ifndef OS_DEBUG_USART_BRR
#define OS_DEBUG_USART_BRR    (*(volatile uint32_t*)(OS_DEBUG_USART_BASE + 0x08UL))
#endif
#ifndef OS_DEBUG_USART_CR1
#define OS_DEBUG_USART_CR1    (*(volatile uint32_t*)(OS_DEBUG_USART_BASE + 0x0CUL))
#endif

/* TX GPIO (PA9 = USART1_TX on STM32F1) */
#ifndef OS_DEBUG_TX_GPIO_BASE
#define OS_DEBUG_TX_GPIO_BASE 0x40010800UL   /* GPIOA on STM32F1 */
#endif
#ifndef OS_DEBUG_TX_PIN
#define OS_DEBUG_TX_PIN       9
#endif
#ifndef OS_DEBUG_TX_CRH_OFFSET
/* STM32F1 uses CRH for pins 8-15, CRL for pins 0-7 */
#define OS_DEBUG_TX_CRH_OFFSET 0x04UL
#endif

/* Baud rate */
#ifndef OS_DEBUG_BAUDRATE
#define OS_DEBUG_BAUDRATE     115200UL
#endif

/* ═══════════════ Output Mode (UART only) ═══════════════
 * 0 = polling (simple, blocking TX with timeout)
 * 1 = interrupt-driven ring buffer (non-blocking TX)
 */
#ifndef OS_DEBUG_RING_BUFFER
#define OS_DEBUG_RING_BUFFER  0
#endif

#if OS_DEBUG_RING_BUFFER
#ifndef OS_DEBUG_RING_SIZE
#define OS_DEBUG_RING_SIZE    256
#endif
#endif

#endif /* OS_DEBUG_BACKEND == UART */

/* ═══════════════ RTT Backend Configuration ═══════════════
 * SEGGER RTT-compatible ring buffer on SRAM — no physical pin needed.
 * Uses SWD debug probe + RTT viewer (Ozone, J-Link RTT Viewer, etc.)
 *
 * The RTT control block is placed at a known address so the debug
 * probe can find it. Override OS_RTT_BUFFER_ADDR if needed.
 */
#if (OS_DEBUG_BACKEND == OS_DEBUG_BACKEND_RTT)
#ifndef OS_RTT_BUFFER_SIZE
#define OS_RTT_BUFFER_SIZE    1024
#endif
#ifndef OS_RTT_BUFFER_ADDR
/* Default: place at start of a dedicated SRAM section.
   In linker script, reserve 1KB: .rtt_buffer (NOLOAD) */
#define OS_RTT_BUFFER_ADDR    0x20000000UL
#endif
#endif /* OS_DEBUG_BACKEND == RTT */

/* ═══════════════ Log Levels ═══════════════
 * Compile-time filtering: set OS_DEBUG_MAX_LEVEL to suppress
 * lower-level messages entirely from the binary.
 *   0 = ERROR   (critical faults, stack overflow, HardFault)
 *   1 = WARN    (deadline miss, watchdog, degraded operation)
 *   2 = INFO    (context switch, task create/stop, boot banner)
 *   3 = TRACE   (detailed per-tick, per-event tracing)
 */
#ifndef OS_DEBUG_MAX_LEVEL
#define OS_DEBUG_MAX_LEVEL    2
#endif

#define OS_DEBUG_LEVEL_ERROR  0
#define OS_DEBUG_LEVEL_WARN   1
#define OS_DEBUG_LEVEL_INFO   2
#define OS_DEBUG_LEVEL_TRACE  3

/* ═══════════════ Periodic Stats ═══════════════
 * Enable periodic statistics output (CPU usage, stack watermarks).
 * 0 = disabled, 1 = enabled.
 */
#ifndef OS_DEBUG_STATS
#define OS_DEBUG_STATS        0
#endif

#if OS_DEBUG_STATS
#ifndef OS_DEBUG_STATS_INTERVAL_MS
#define OS_DEBUG_STATS_INTERVAL_MS  10000UL
#endif
#endif

/* ═══════════════ Public API ═══════════════ */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the debug UART hardware (GPIO + USART registers).
 * Safe to call from os_init() or before os_start().
 * Does nothing when OS_DEBUG_UART=0.
 */
void os_debug_init(void);

/**
 * Send a single byte over the debug UART (polling).
 * Drops the byte if TX is not ready — never blocks indefinitely.
 */
void os_debug_putc(char c);

/**
 * Send a null-terminated string over the debug UART.
 */
void os_debug_puts(const char* str);

/**
 * Lightweight printf-like formatted output.
 * Supported format specifiers:
 *   %d  — signed decimal integer
 *   %u  — unsigned decimal integer
 *   %x  — lowercase hex
 *   %X  — uppercase hex
 *   %s  — null-terminated string
 *   %c  — single character
 *   %%  — literal '%'
 *
 * Field width and zero-padding supported: %08x, %4d, etc.
 * Maximum format string length: 128 bytes.
 */
void os_debug_printf(const char* fmt, ...);

/**
 * Log a kernel event at the specified level.
 * Automatically prefixes with [E]/[W]/[I]/[T] and current tick.
 */
void os_debug_log(uint8_t level, const char* fmt, ...);

/**
 * Print the boot banner (OS family, version, clock, architecture).
 */
void os_debug_boot_banner(void);

/**
 * Print a context switch trace line.
 */
void os_debug_context_switch(const char* from_name, uint8_t from_id,
                              const char* to_name, uint8_t to_id);

/**
 * Print a fault dump (CFSR, HFSR, MMFAR, BFAR, task info, registers).
 */
void os_debug_fault_dump(void);

/**
 * Print periodic statistics (CPU usage, stack watermarks, deadline misses).
 * Called from os_tick when OS_DEBUG_STATS=1 and interval has elapsed.
 */
void os_debug_stats(void);

#ifdef __cplusplus
}
#endif

/* ═══════════════ Convenience Macros ═══════════════ */

#define OS_DLOG_ERROR(...)  os_debug_log(OS_DEBUG_LEVEL_ERROR, __VA_ARGS__)
#define OS_DLOG_WARN(...)   os_debug_log(OS_DEBUG_LEVEL_WARN, __VA_ARGS__)
#define OS_DLOG_INFO(...)   os_debug_log(OS_DEBUG_LEVEL_INFO, __VA_ARGS__)
#define OS_DLOG_TRACE(...)  os_debug_log(OS_DEBUG_LEVEL_TRACE, __VA_ARGS__)

#else /* OS_DEBUG_UART == 0 */

/* All debug functions compile to empty inline stubs */
static inline void os_debug_init(void) {}
static inline void os_debug_putc(char) {}
static inline void os_debug_puts(const char*) {}
/* os_debug_printf not available when debug is off (no varargs overhead) */

#define OS_DLOG_ERROR(...)  ((void)0)
#define OS_DLOG_WARN(...)   ((void)0)
#define OS_DLOG_INFO(...)   ((void)0)
#define OS_DLOG_TRACE(...)  ((void)0)

#endif /* OS_DEBUG_UART */
