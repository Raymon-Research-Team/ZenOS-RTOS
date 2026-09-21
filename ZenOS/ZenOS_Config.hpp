#pragma once
/**
 * @file    ZenOS_Config.hpp
 * @brief   ZenOS RTOS Configuration
 *
 * This is the ONLY file users modify for behavior settings.
 * Resource sizes are calculated automatically.
 * Hardware detection is in ZenOS_Port.hpp.
 *
 * Naming convention:
 *   OS_KERNEL_*   — core timing and memory
 *   OS_IPC_TOOLS_EN  — scheduling and communication primitives
 *   OS_MONITOR_*  — runtime observability
 *   OS_SAFETY_*   — fault detection and hardware protection
 *
 * ── Safety Standard Annotations ──────────────────────────────────
 * Each config option is annotated with the safety standards it
 * relates to.  Two macros at the bottom of this file enable
 * compile-time enforcement of mandatory requirements:
 *
 *   OS_TARGET_MEDICAL_CLASS  — IEC 62304 (SW lifecycle) + IEC 60601-1 (medical EE)
 *                        Software safety classes:  A (lowest risk)
 *                                                  B (non-serious injury)
 *                                                  C (death or serious injury)
 *   OS_TARGET_INDUSTRIAL_SIL — IEC 61508 (functional safety) + IEC 61511 (process)
 *                          SIL levels: 1 (lowest) … 4 (highest)
 *
 * Legend for per-option annotations:
 *   [MED-A]  Medical IEC 62304 Class A — all options recommended
 *   [MED-B]  Medical IEC 62304 Class B — fault detection required
 *   [MED-C]  Medical IEC 62304 Class C — fault detection + tolerance required
 *   [IND-1]  Industrial IEC 61508 SIL 1 — basic fault detection
 *   [IND-2]  Industrial IEC 61508 SIL 2 — robust fault detection
 *   [IND-3]  Industrial IEC 61508 SIL 3 — high-integrity fault tolerance
 *   [IND-4]  Industrial IEC 61508 SIL 4 — highest-integrity (rare in SW)
 *
 * @author  Rahman Heidari <rahman.h22@gmail.com> — Raymon Research Team
 * @version 1.1.0
 */


/* ============================================================================
 *  OS_KERNEL — Core Timing and Memory
 * ============================================================================ */

/* Tick resolution in microseconds (100..1000, must divide 1000 evenly)
 * [MED-B] [MED-C] [IND-1] [IND-2] Must be ≤ 1 ms for Class B/C and SIL ≥ 1.
 *          Medical: IEC 62304 requires timing analysis for Class B/C;
 *          a tick ≤ 1 ms is necessary for deadline verification.
 *          Industrial: IEC 61508 Part 3 Table 4 — SW response time
 *          must be demonstrably less than the process safety time. */
#ifndef OS_KERNEL_TICK_PERIOD_US
#define OS_KERNEL_TICK_PERIOD_US  100UL // Default=100
#endif

#if (OS_KERNEL_TICK_PERIOD_US < 100UL) || (OS_KERNEL_TICK_PERIOD_US > 1000UL)
#error "OS_KERNEL_TICK_PERIOD_US must be 100..1000"
#endif

#if (1000UL % OS_KERNEL_TICK_PERIOD_US) != 0
#error "OS_KERNEL_TICK_PERIOD_US must divide 1000 evenly"
#endif
//-----------------------------------------------------------------------------

/* Compiler optimization level string for the banner.
   GCC's __OPTIMIZE__ macro is 1 for ALL levels >= O1 (O1, O2, O3, Og, Os),
   so it cannot distinguish between them.  Set this manually in your
   project's preprocessor defines to get the correct banner:
       -DOS_OPT_LEVEL_STRING="O2"
   If not defined, the banner shows "O+" for any optimized build.
 * [MED-C] [IND-3] Compiler optimization must be fixed and documented;
 *          different optimization levels may change stack layout and timing.
 *          IEC 62304 §5.5 requires reproducible builds. */
/* #define OS_OPT_LEVEL_STRING "O2" */

/* Default stack size per task in bytes.
   512 instead of 256: at -O2 the optimizer inlines HAL/library frames
   deeper than at -O0, and 256 left almost no margin (overflow would
   corrupt adjacent TCBs silently).  Shrink back if RAM is tight — the
   per-task peak-stack report in the test summary shows real usage.
 * [MED-B] [MED-C] [IND-1] [IND-2] [IND-3] Stack sizes must be verified
 *          by static analysis or runtime measurement.  IEC 62304 §5.4.3
 *          requires analysis of resource usage.  IEC 61508 Part 3 §7.4.3
 *          requires stack depth analysis for all execution paths. */
#ifndef OS_KERNEL_DEFAULT_STACK_SIZE
#define OS_KERNEL_DEFAULT_STACK_SIZE  128 // Default=128
#endif
//-----------------------------------------------------------------------------
/**
 * Compile-time scheduler priority configuration
 *
 * OS_KERNEL_MAX_PRIORITIES is the number of scheduler priority slots.
 * Priority 0 is reserved for the idle task and is the LOWEST priority.
 * Usable priorities are 1..OS_KERNEL_MAX_PRIORITIES-1.
 *
 * PRIORITY CONVENTION (matches CMSIS-RTOS2, FreeRTOS, ThreadX):
 *   A LARGER number means a HIGHER priority.
 *   OS_KERNEL_MAX_PRIORITIES-1 is the highest usable priority.
 *   Priority 1 is the lowest usable (user) priority.
 *   Priority 0 is the idle task and never runs while any user task is ready.
 *
 * Default: 16 slots (priorities 1..16, where 16 is highest).
 * Maximum: 255 slots (priorities 1..255, where 255 is highest).
 */
#ifndef OS_KERNEL_MAX_PRIORITIES
#define OS_KERNEL_MAX_PRIORITIES 16 // Default=16
#endif
#if (OS_KERNEL_MAX_PRIORITIES < 2) || (OS_KERNEL_MAX_PRIORITIES > 256)
#error "OS_KERNEL_MAX_PRIORITIES must be between 2 and 256"
#endif
//-----------------------------------------------------------------------------
/* Tickless idle: separate kernel-level flag for power management */
#ifndef OS_KERNEL_TICKLESS_IDLE_EN
#define OS_KERNEL_TICKLESS_IDLE_EN  1 // Default=1 (runs in idle task)
#endif



/* ============================================================================
 *  OS_TOOLS — Unified Scheduling and Communication Primitives
 * -------------------------------------------------------------------------- *
 *  One macro to rule them all: enables all inter-task communication features.
 *
 *  1 = enabled (EVENT + MUTEX + QUEUE + SEMAPHORE)
 *  0 = disabled (all IPC reduced to no-op functions)
 *
 *  IEC 62304/Medical & IEC 61508/Industrial safety standards:
 *   [MED-B] [MED-C] [IND-2] [IND-3] Inter-task communication primitives
 *          are required when multiple tasks share resources or need
 *          coordinated execution.  Priority inheritance (OS_MUTEX) is
 *          required to prevent unbounded priority inversion.
 *          IEC 62304 §5.4.5 — data integrity between software items.
 *          IEC 61508 Part 3 §7.4.13 — avoidance of deadlock and livelock.
 * ============================================================================ */

#ifndef OS_IPC_TOOLS_EN
#define OS_IPC_TOOLS_EN    1 // Default=1 (ENABLE ALL IPC)
#endif

/* ============================================================================
 *  OS_MONITORING_EN — Runtime Observability (single master switch)
 * -------------------------------------------------------------------------- *
 *  One switch for the whole monitoring subsystem: 1 = enabled, 0 = disabled.
 *  The previous per-feature switches (OS_MONITOR_DEADLINE,
 *  OS_MONITOR_TCB_INTEGRITY, OS_MONITOR_ERROR_LOG) were merged into this
 *  single macro — the features below are enabled and disabled together.
 *
 *  Setting OS_MONITORING_EN=1 compiles in and runs:
 *
 *    1. Deadline monitoring
 *       - os_task_set_deadline() arms a per-task hard deadline (the TCB
 *         gains deadline_ticks / deadline_miss_count fields)
 *       - os_tick() detects misses, counts them and reports DEADLINE_MISS;
 *         the reaction is configured by OS_MONITORING_DEADLINE_ACTION
 *       - os_get_deadline_miss_count() exposes the per-task miss count
 *
 *    2. TCB integrity checking
 *       - every TCB carries a magic number (OS_TCB_MAGIC) written at task
 *         creation and verified before watchdog recovery actions
 *       - a corrupted TCB is reported as OSError::TCB_CORRUPTED and the
 *         affected task is deactivated instead of reset
 *
 *    3. Error log (RAM ring buffer)
 *       - OS_MONITORING_ERROR_LOG_SIZE entries, each with timestamp, error code,
 *         task ID and severity
 *       - _os_report_error() logs every kernel error automatically
 *       - query API: os_log_error(), os_get_error_log_entry(),
 *         os_get_error_log_count(), os_get_error_log_total()
 *
 *    4. CPU load and stack instrumentation
 *       - os_get_cpu_usage() / os_get_cpu_usage_total() /
 *         os_get_task_cpu_usage()
 *       - per-task peak-SP watermark tracking, os_get_stack_watermark() /
 *         os_get_stack_watermark_percent() and the os_get_stack_report()
 *         table printed by the self-test
 *
 *  Setting OS_MONITORING_EN=0 removes all of the above from the build: the TCB
 *  loses those fields, the error-log API becomes a no-op stub and the
 *  monitor query functions are not compiled (application code guards its
 *  calls with #if OS_MONITORING_EN).
 *
 *  RAM cost when enabled (Cortex-M3, approximate): ~8 bytes per error-log
 *  entry (OS_MONITORING_ERROR_LOG_SIZE entries) + 16 bytes per task (deadline,
 *  magic and watermark fields).
 *
 *  IEC enforcement: OS_TARGET_MEDICAL_CLASS >= 2 and OS_TARGET_INDUSTRIAL_SIL >= 1
 *  force this switch on at compile time (see Safety Standard Enforcement
 *  at the bottom of this file).
 *
 *  [MED-B] [MED-C] REQUIRED — IEC 62304 §5.4.3 (timing analysis /
 *          deadline evidence), §5.4.4 (control-data integrity) and
 *          §5.5.4 (logging and tracing of safety-related events)
 *          for Class B/C.
 *  [IND-1] [IND-2] [IND-3] REQUIRED — IEC 61508 Part 1 §7.4.7 (event
 *          recording), Part 2 Table 3 (control-data integrity) and
 *          Part 3 Table 5 (timing diagnostics).
 * ============================================================================ */

#ifndef OS_MONITORING_EN
#define OS_MONITORING_EN             0 // Default=0
#endif
//-----------------------------------------------------------------------------
/* Reaction to a detected deadline miss (sub-option of OS_MONITORING_EN):
 * 0 = log only, 1 = reset task, 2 = disable task (safe state)
 * [MED-B] [MED-C] REQUIRED ≥ 1 — IEC 62304 §5.4.7: fault reaction.
 *          Logging alone (0) is insufficient for Class B/C; the system
 *          must take corrective action (reset or disable).
 * [IND-2] [IND-3] REQUIRED ≥ 1 — IEC 61508 Part 1 §7.4.8:
 *          safety function must respond to detected faults.
 *          Option 1 (reset) enables recovery; option 2 (disable)
 *          enters a safe state. */
#ifndef OS_MONITORING_DEADLINE_ACTION
#define OS_MONITORING_DEADLINE_ACTION      1 // Default=1
#endif

#if (OS_MONITORING_DEADLINE_ACTION < 0) || (OS_MONITORING_DEADLINE_ACTION > 2)
#error "OS_MONITORING_DEADLINE_ACTION must be 0, 1, or 2"
#endif
//-----------------------------------------------------------------------------
/* Error log capacity in entries (sub-option of OS_MONITORING_EN).  The ring
 * buffer is RAM-only and lost on reset — persist logs to non-volatile
 * storage for production use if post-mortem traces are needed.
 * [MED-C] [IND-2] [IND-3] Minimum capacity should be sufficient to
 *          capture all errors during a worst-case operating cycle.
 *          16 entries (~128 bytes of RAM) is the default: it captures a
 *          full fault burst of a typical safety cycle while keeping the
 *          RAM cost low on small parts (F103 = 20 KB).  Increase for long
 *          cycle times or higher diagnostic coverage requirements. */
#ifndef OS_MONITORING_ERROR_LOG_SIZE
#define OS_MONITORING_ERROR_LOG_SIZE    16 // Default=16
#endif

#if OS_MONITORING_ERROR_LOG_SIZE < 1
#error "OS_MONITORING_ERROR_LOG_SIZE must be at least 1"
#endif

/* ============================================================================
 *  OS_SAFETY — Fault Detection and Hardware Protection
 * -------------------------------------------------------------------------- *
 *  1 = enabled, 0 = disabled.  Each feature compiles independently.
 * ============================================================================ */

/* RAM test: background March C- algorithm for SRAM integrity
 * [MED-C] REQUIRED — IEC 62304 §5.4.4: memory integrity verification
 *          for safety-related data.  Single-bit faults in SRAM can
 *          corrupt control variables without detection.
 * [IND-3] [IND-4] REQUIRED — IEC 61508 Part 2 Table 2: diagnostic
 *          coverage for random hardware faults in RAM.  March C-
 *          provides stuck-at fault detection.
 * NOTE: This test is non-destructive but may produce false positives
 *       during active DMA transfers.  Call from idle task only. */
#ifndef OS_SAFETY_RAM_TEST_EN
#define OS_SAFETY_RAM_TEST_EN     0 // Default=0
#endif
//-----------------------------------------------------------------------------
/* MPU: hardware memory protection per task (Cortex-M3/M4/M7)
 * [MED-C] REQUIRED — IEC 62304 §5.4.4: spatial isolation between
 *          safety-related and non-safety software items.
 *          Prevents a faulty task from corrupting another task's memory.
 * [IND-3] REQUIRED — IEC 61508 Part 2 Table 2: spatial partitioning
 *          is recommended for SIL 3 to contain faults within modules.
 * NOTE: Requires ARM Cortex-M3+ with MPU.  Cortex-M0/L0 do not
 *       have MPU hardware; set to 0 on those targets. */
#ifndef OS_SAFETY_MPU_EN
#define OS_SAFETY_MPU_EN    0 // Default=0
#endif
//-----------------------------------------------------------------------------
/* Hardware watchdog feed/check integration
 * [MED-C] REQUIRED — IEC 62304 §5.4.7: fault tolerance requires
 *          a mechanism to recover from unrecoverable SW faults.
 *          The HW watchdog resets the MCU if the SW hangs.
 * [IND-2] [IND-3] REQUIRED — IEC 61508 Part 2 Table 2: process
 *          interruption / replacement (watchdog as external monitor).
 *          IEC 61508 Part 3 §7.4.14: external monitoring device.
 * NOTE: Configure IWDG timeout via CubeMX.  The feed task must run
 *       at ≤ 50% of the IWDG timeout to prevent spurious resets. */
#ifndef OS_SAFETY_HW_WATCHDOG_EN
#define OS_SAFETY_HW_WATCHDOG_EN  0 // Default=0
#endif
//-----------------------------------------------------------------------------
/* CRC check: ROM integrity verification via CRC peripheral
 * [MED-C] REQUIRED — IEC 62304 §5.4.4: code integrity verification.
 *          Detects flash bit-flips from radiation or electrical stress.
 * [IND-3] [IND-4] REQUIRED — IEC 61508 Part 2 Table 2: diagnostic
 *          coverage for program memory.  Periodic CRC re-verification
 *          detects latent faults in executable code.
 * NOTE: Do not write to flash while CRC check is in progress;
 *       this causes false positives. */
#ifndef OS_SAFETY_CRC_EN
#define OS_SAFETY_CRC_EN   0 // Default=0
#endif
//-----------------------------------------------------------------------------
/* Software watchdog: detect stuck tasks and recover
 * [MED-B] [MED-C] REQUIRED — IEC 62304 §5.4.6: error detection
 *          for safety-related tasks.  A stuck task is a failure
 *          mode that must be detected within the safety time.
 * [IND-1] [IND-2] REQUIRED — IEC 61508 Part 3 Table 5: SW fault
 *          detection for diagnostic coverage calculation.
 * NOTE: The timeout must be set to ≤ the process safety time.
 *       Tasks that legitimately run long must yield periodically. */
#ifndef OS_SAFETY_SOFT_WATCHDOG_EN
#define OS_SAFETY_SOFT_WATCHDOG_EN     0 // Default=0
#endif
//-----------------------------------------------------------------------------
/* Max task recovery attempts before permanent disable
 * [MED-C] [IND-3] Recommended ≥ 1 — IEC 62304 §5.4.7 and
 *          IEC 61508 Part 1 §7.4.8: after repeated failures,
 *          the system should enter a safe state (task disabled)
 *          rather than continue attempting recovery indefinitely.
 *          Set to 0 to disable recovery (immediate safe state). */
#ifndef OS_SAFETY_TASK_MAX_RECOVERY
#define OS_SAFETY_TASK_MAX_RECOVERY     3 // Default=3
#endif
//-----------------------------------------------------------------------------
/* Software watchdog timeout in milliseconds
 * [MED-B] [MED-C] REQUIRED — Must be ≤ the application's safety time.
 *          IEC 62304 §5.4.6: error detection time must be less than
 *          the time to reach a hazardous state.
 * [IND-1] [IND-2] REQUIRED — Must be ≤ the process safety time.
 *          IEC 61508 Part 1 §7.4.4: diagnostic test interval.
 *          Common values: 500 ms (motor control), 3000 ms (slow process).
 *          Reduce for faster-responding safety functions. */
#ifndef OS_SAFETY_SOFT_WDG_TIMEOUT_MS
#define OS_SAFETY_SOFT_WDG_TIMEOUT_MS  3000UL // Default=3000UL
#endif
//-----------------------------------------------------------------------------
/* Max duration (us) allowed inside OS_SAFE before reporting error
 * [MED-C] [IND-3] Recommended — Long critical sections increase
 *          interrupt latency, which can cause deadline misses.
 *          IEC 61508 Part 3 §7.4.13: worst-case interrupt latency
 *          must be bounded and documented.
 *          Set to 0 to disable the check. */
#ifndef OS_SAFETY_MAX_CRITICAL_US
#define OS_SAFETY_MAX_CRITICAL_US  1000UL // Default=1000UL
#endif

/* ============================================================================
 *  Safety Standard Enforcement
 * -------------------------------------------------------------------------- *
 *  Define OS_TARGET_MEDICAL_CLASS to enforce IEC 62304 (Medical SW Lifecycle)
 *  Define OS_TARGET_INDUSTRIAL_SIL to enforce IEC 61508 (Functional Safety)
 *  These can be combined — the stricter requirement applies.
 *
 *  Usage:
 *      -DOS_TARGET_MEDICAL=2        // IEC 62304 Class A (set 1, 2, or 3)
 *      -DOS_TARGET_INDUSTRIAL=3     // IEC 61508 SIL (set 1, 2, 3, or 4)
 *
 *  Override in project preprocessor defines, NOT in this file.
 * ============================================================================ */

/* ── IEC 62304 Medical Software Safety Classes ──
 *   Class A: No injury possible (informational, non-critical)
 *   Class B: Non-serious injury possible
 *   Class C: Death or serious injury possible
 *>
 * ── IEC 61508 Industrial Safety Integrity Levels ──
 *   SIL 1: Low risk — basic fault detection
 *   SIL 2: Medium risk — robust fault detection + reaction
 *   SIL 3: High risk — high-integrity fault tolerance
 *   SIL 4: Very high risk — typically hardware, not SW
 */

#ifndef OS_TARGET_MEDICAL_CLASS
#define OS_TARGET_MEDICAL_CLASS  0   /* 0 = not targeting medical */
#endif
#if (OS_TARGET_MEDICAL_CLASS < 0) || (OS_TARGET_MEDICAL_CLASS > 3)
#error "OS_TARGET_MEDICAL_CLASS must be 0 (disabled), 1 (Class A), 2 (Class B), or 3 (Class C)"
#endif
//-----------------------------------------------------------------------------
#ifndef OS_TARGET_INDUSTRIAL_SIL
#define OS_TARGET_INDUSTRIAL_SIL  0  /* 0 = not targeting industrial */
#endif
#if (OS_TARGET_INDUSTRIAL_SIL < 0) || (OS_TARGET_INDUSTRIAL_SIL > 4)
#error "OS_TARGET_INDUSTRIAL_SIL must be 0 (disabled), 1 (SIL 1), 2 (SIL 2), 3 (SIL 3), or 4 (SIL 4)"
#endif

/* ═══════════════════════════════════════════════════════════════════
 *  IEC 62304 Enforcement (Medical)
 * ═══════════════════════════════════════════════════════════════════ */

/* ── Class B and above: fault detection required ── */
#if (OS_TARGET_MEDICAL_CLASS >= 2)

#if !OS_MONITORING_EN
#error "[IEC 62304 Class B/C] OS_MONITORING_EN must be enabled — deadline monitoring, TCB integrity and error logging are required for fault detection and traceability"
#endif

#if !OS_SAFETY_SOFT_WATCHDOG_EN
#error "[IEC 62304 Class B/C] OS_SAFETY_SOFT_WATCHDOG_EN must be enabled — task fault detection is required"
#endif

#if (OS_MONITORING_DEADLINE_ACTION < 1)
#error "[IEC 62304 Class B/C] OS_MONITORING_DEADLINE_ACTION must be ≥ 1 — logging alone is insufficient, corrective action required"
#endif

#endif /* OS_TARGET_MEDICAL_CLASS >= 2 */

/* ── Class C: fault detection + tolerance + memory protection required ── */
#if (OS_TARGET_MEDICAL_CLASS >= 3)

#if !OS_SAFETY_HW_WATCHDOG_EN
#error "[IEC 62304 Class C] OS_SAFETY_HW_WATCHDOG_EN must be enabled — HW watchdog is required for fault tolerance"
#endif

/* Control-data integrity (TCB magic) is enforced by the OS_MONITORING_EN
   requirement in the Class B/C block above. */

#if !OS_SAFETY_MPU_EN
#error "[IEC 62304 Class C] OS_SAFETY_MPU_EN must be enabled — spatial isolation required for safety partitioning"
#endif

#if !OS_SAFETY_CRC_EN
#error "[IEC 62304 Class C] OS_SAFETY_CRC_EN must be enabled — code integrity verification required"
#endif

#if !OS_SAFETY_RAM_TEST_EN
#error "[IEC 62304 Class C] OS_SAFETY_RAM_TEST_EN must be enabled — RAM integrity verification required"
#endif

#endif /* OS_TARGET_MEDICAL_CLASS >= 3 */


/* ═══════════════════════════════════════════════════════════════════
 *  IEC 61508 Enforcement (Industrial)
 * ═══════════════════════════════════════════════════════════════════ */

/* ── SIL 1+: basic fault detection ── */
#if (OS_TARGET_INDUSTRIAL_SIL >= 1)

#if !OS_SAFETY_SOFT_WATCHDOG_EN
#error "[IEC 61508 SIL 1+] OS_SAFETY_SOFT_WATCHDOG_EN must be enabled — SW fault detection required"
#endif

#if !OS_MONITORING_EN
#error "[IEC 61508 SIL 1+] OS_MONITORING_EN must be enabled — event recording required for diagnostics"
#endif

#endif /* OS_TARGET_INDUSTRIAL_SIL >= 1 */

/* ── SIL 2+: robust fault detection + reaction ── */
#if (OS_TARGET_INDUSTRIAL_SIL >= 2)

/* Timing diagnostics are enforced by the OS_MONITORING_EN requirement in the
   SIL 1+ block above. */

#if !OS_IPC_TOOLS_EN
#error "[IEC 61508 SIL 2+] OS_IPC_TOOLS_EN must be enabled — mutual exclusion required for shared resources"
#endif

#if !OS_SAFETY_HW_WATCHDOG_EN
#error "[IEC 61508 SIL 2+] OS_SAFETY_HW_WATCHDOG_EN must be enabled — external monitoring device required"
#endif

#if (OS_MONITORING_DEADLINE_ACTION < 1)
#error "[IEC 61508 SIL 2+] OS_MONITORING_DEADLINE_ACTION must be ≥ 1 — corrective action on fault required"
#endif

#endif /* OS_TARGET_INDUSTRIAL_SIL >= 2 */

/* ── SIL 3+: high-integrity fault tolerance ── */
#if (OS_TARGET_INDUSTRIAL_SIL >= 3)

#if !OS_SAFETY_MPU_EN
#error "[IEC 61508 SIL 3+] OS_SAFETY_MPU_EN must be enabled — spatial partitioning required for fault containment"
#endif

#if !OS_SAFETY_RAM_TEST_EN
#error "[IEC 61508 SIL 3+] OS_SAFETY_RAM_TEST_EN must be enabled — RAM diagnostic coverage required"
#endif

#if !OS_SAFETY_CRC_EN
#error "[IEC 61508 SIL 3+] OS_SAFETY_CRC_EN must be enabled — program memory verification required"
#endif

/* Control-data integrity is enforced by the OS_MONITORING_EN requirement in the
   SIL 1+ block above. */

#endif /* OS_TARGET_INDUSTRIAL_SIL >= 3 */
