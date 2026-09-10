# ZenOS RTOS — Complete Configuration Guide

This is the **single reference for every ZenOS configuration option**. All settings live in
[`ZenOS/ZenOS_Config.hpp`](../ZenOS/ZenOS_Config.hpp) — the only file users edit for behavior settings.
Override any option via compiler `-D` flags or before the `#ifndef` guards in the header.

**Naming convention:**
- `OS_KERNEL_*` — core timing and memory
- `OS_TOOL_*` — scheduling and communication primitives
- `OS_MONITOR_*` — runtime observability
- `OS_SAFETY_*` — fault detection and hardware protection

**Legend for safety annotations:** [MED-A/B/C] = IEC 62304 medical classes · [IND-1..4] = IEC 61508 SIL levels.
Compile-time enforcement is covered in [§9](#9-compile-time-enforcement-of-safety-standards).

---

## 1. Kernel — Core Timing and Memory

### 1.1 `OS_KERNEL_TICK_PERIOD_US` — Tick resolution
| | |
|---|---|
| **Default** | `100` µs (10 kHz tick) |
| **Range** | 100–1000, must divide 1000 evenly (compile-time checked) |
| **Safety** | [MED-B] [MED-C] [IND-1] [IND-2] |

Tick resolution in microseconds. Must be ≤ 1 ms for medical Class B/C and SIL ≥ 1
(IEC 62304 requires timing analysis for Class B/C; IEC 61508 Part 3 Table 4 requires the SW
response time to be demonstrably less than the process safety time).

1 tick = `OS_KERNEL_TICK_PERIOD_US` µs, so `os_delay_ms(ms)` converts to `ms * OS_TICKS_PER_MS` ticks.

### 1.2 `OS_KERNEL_STACK_SIZE` — Default task stack
| | |
|---|---|
| **Default** | `512` bytes |
| **Range** | ≥ 64 |
| **Safety** | [MED-B] [MED-C] [IND-1] [IND-2] [IND-3] |

Default stack size per task in bytes. 512 (instead of 256) leaves margin for inlined HAL/library
frames at -O2; the per-task peak-stack report in the test summary shows real usage — shrink if RAM
is tight. IEC 62304 §5.4.3 requires resource-usage analysis; IEC 61508 Part 3 §7.4.3 requires stack
depth analysis for all execution paths. Rule of thumb: set to ≥130% of measured peak
(`os_get_stack_usage()`, `os_get_stack_report()`).

### 1.3 `OS_KERNEL_MAX_PRIORITIES` — Scheduler priority slots
| | |
|---|---|
| **Default** | `32` |
| **Range** | 2–256 (compile-time checked) |
| **Safety** | — |

Number of scheduler priority slots. Priority 0 is reserved (idle); usable priorities are
`1..OS_KERNEL_MAX_PRIORITIES-1`.

**Scheduler Priority Model:** every usable priority is a distinct scheduler level — no priority
banding, no `priority >> 3` mapping.

| Slots | Usable priorities | Queue heads | Bitmap | Base priority storage |
|---:|---:|---:|---:|---:|
| 32 | 1..31 | 128 B | 4 B | 132 B |
| 64 | 1..63 | 256 B | 8 B | 264 B |
| 128 | 1..127 | 512 B | 16 B | 528 B |
| 256 | 1..255 | 1024 B | 32 B | 1056 B |

The default 32-priority build keeps the scalar bitmap and 32 queue-head objects, so configurable
priorities add no RAM by default. Priorities beyond 32 use extension storage sized by the derived
macros `OS_PRIORITY_BITMAP_WORDS`, `OS_PRIORITY_EXTRA_COUNT` and `OS_PRIORITY_EXTRA_WORDS`
(declared in `ZenOS.hpp` — calculated values, not user settings) compiled only when
`OS_KERNEL_MAX_PRIORITIES > 32`. The round-robin throttle extension follows the same rule.

**Validation:** task creation rejects a non-zero priority outside the configured range before
inserting the task into a priority queue; priority `0` is normalized to `1` as before.

---

## 2. Tools — Scheduling and Communication Primitives

All options are `1` (enabled) / `0` (disabled), each compiles independently.

### 2.1 `OS_TOOL_EVENT` — Inter-task signaling
| | |
|---|---|
| **Default** | `1` |
| **Safety** | [MED-B] [MED-C] [IND-2] |

Binary/counting events. Required for inter-task communication in systems with more than one active
task (IEC 62304 §5.4.5 — data integrity between software items).

### 2.2 `OS_TOOL_MUTEX` — Mutual exclusion with priority inheritance
| | |
|---|---|
| **Default** | `1` |
| **Safety** | [MED-B] [MED-C] [IND-2] [IND-3] |

Priority inheritance prevents unbounded priority inversion (IEC 61508 Part 3 §7.4.13 — avoidance
of deadlock/livelock). Required for shared resources.

### 2.3 `OS_TOOL_QUEUE` — Bounded FIFO queue
| | |
|---|---|
| **Default** | `1` |
| **Safety** | [MED-B] [IND-2] |

Template-based, compile-time capacity for typed bounded data transfer (IEC 62304 §5.4.4 — defined
data interfaces).

### 2.4 `OS_TOOL_SEMAPHORE` — Counting semaphore
| | |
|---|---|
| **Default** | `1` |
| **Safety** | [MED-B] [IND-2] |

With max-count for bounded resource pools and ISR-to-task signaling (IEC 61508 Part 3 §7.4.11 —
bounded resource usage).

### 2.5 `OS_TOOL_TICKLESS_IDLE` — WFI-based idle
| | |
|---|---|
| **Default** | `1` |
| **Safety** | [MED-A] optional |

Tickless idle for power savings. Disable if a deterministic power profile is required.

---

## 3. Monitor — Runtime Observability

### 3.1 `OS_MONITOR_DEADLINE` — Deadline monitoring
| | |
|---|---|
| **Default** | `0` |
| **Range** | 0/1 |
| **Safety** | [MED-B] [MED-C] [IND-2] [IND-3] |

Detects and acts on task deadline misses. REQUIRED for IEC 62304 Class B/C (§5.4.3) and IEC 61508
SIL 2+ (Part 3 Table 5 — diagnostic coverage of execution timing). Deadline misses indicate a
potential failure of the safety function. When enabled, the `TCB` gains `deadline_ticks` and
`deadline_miss_count` fields; set deadlines with `os_task_set_deadline()` and read misses with
`os_get_deadline_miss_count()`.

> **Note:** the SAFETY_MANUAL's historical table lists this as default 1; the code default is 0.
> Enable it (or `-DOS_MONITOR_DEADLINE=1`) whenever tasks have deadlines. It is auto-enforced
> (`#error`) when targeting Medical Class B/C or Industrial SIL 2+.

### 3.2 `OS_MONITOR_TCB_INTEGRITY` — TCB magic-number check
| | |
|---|---|
| **Default** | `0` |
| **Range** | 0/1 |
| **Safety** | [MED-C] [IND-3] |

Magic-number check for memory-corruption detection (IEC 62304 §5.4.4; IEC 61508 Part 2 Table 3 —
random hardware fault metrics for control data). Adds `magic`/`overflow_count` to the `TCB`. Code
default is 0; required (enforced) for Medical Class C and SIL 3+.

### 3.3 `OS_MONITOR_ERROR_LOG` — Error log ring buffer
| | |
|---|---|
| **Default** | `1` |
| **Range** | 0/1 |
| **Safety** | [MED-B] [MED-C] [IND-1] [IND-2] [IND-3] |

Circular buffer recording errors with timestamp, task ID and severity (IEC 62304 §5.5.4 — logging
and tracing; IEC 61508 Part 1 §7.4.7 — event recording). The ring buffer is RAM-only and lost on
reset — persist to non-volatile storage for production use.

### 3.4 `OS_MONITOR_ERROR_LOG_SIZE` — Error log capacity
| | |
|---|---|
| **Default** | `32` entries |
| **Range** | ≥ 1 |
| **Safety** | [MED-C] [IND-2] [IND-3] |

Minimum capacity should cover all errors during a worst-case operating cycle; increase for long
cycle times.

---

## 4. Safety — Fault Detection and Hardware Protection

### 4.1 `OS_SAFETY_RAM_TEST` — Background March C- RAM test
| | |
|---|---|
| **Default** | `0` |
| **Range** | 0/1 |
| **Safety** | [MED-C] [IND-3] [IND-4] |

Background March C- algorithm for SRAM integrity (IEC 62304 §5.4.4; IEC 61508 Part 2 Table 2 —
diagnostic coverage for random hardware faults in RAM). Non-destructive, but may produce false
positives during active DMA transfers — call `os_ram_test_step()` from the idle task only.
Code default is 0; required (enforced) for Medical Class C and SIL 3+.

### 4.2 `OS_SAFETY_MPU` — Hardware memory protection
| | |
|---|---|
| **Default** | `0` |
| **Range** | 0/1 |
| **Safety** | [MED-C] [IND-3] |

Per-task MPU protection (Cortex-M3/M4/M7). Spatial isolation between safety-related and
non-safety software (IEC 62304 §5.4.4; IEC 61508 Part 2 Table 2 — spatial partitioning for SIL 3).
Requires ARM Cortex-M3+ (Cortex-M0/L0 have no MPU — set to 0 there). Code default is 0; required
(enforced) for Medical Class C and SIL 3+.

### 4.3 `OS_SAFETY_HW_WATCHDOG` — Hardware watchdog (IWDG)
| | |
|---|---|
| **Default** | `0` |
| **Range** | 0/1 |
| **Safety** | [MED-C] [IND-2] [IND-3] |

OS-level feed/check integration; CubeMX configures the IWDG peripheral itself. Fault tolerance
mechanism (IEC 62304 §5.4.7; IEC 61508 Part 2 Table 2 — external monitoring). The feed task must
run at ≤ 50% of the IWDG timeout. Code default is 0; required (enforced) for Medical Class C and
SIL 2+.

### 4.4 `OS_SAFETY_CRC_CHECK` — ROM integrity via CRC peripheral
| | |
|---|---|
| **Default** | `0` |
| **Range** | 0/1 |
| **Safety** | [MED-C] [IND-3] [IND-4] |

Verifies flash integrity by CRC over program memory, detecting bit-flips from radiation or
electrical stress (IEC 62304 §5.4.4; IEC 61508 Part 2 Table 2). Do not write to flash while the
check is in progress — false positives. Code default is 0; required (enforced) for Medical Class C
and SIL 3+.

### 4.5 `OS_SAFETY_SOFT_WATCHDOG` — Software stuck-task watchdog
| | |
|---|---|
| **Default** | `0` |
| **Range** | 0/1 |
| **Safety** | [MED-B] [MED-C] [IND-1] [IND-2] |

Detects stuck tasks (no yield within `OS_SAFETY_SOFT_WDG_TIMEOUT_MS`) and recovers/resets them
(IEC 62304 §5.4.6; IEC 61508 Part 3 Table 5 — SW fault detection). Timeout must be ≤ the process
safety time; tasks that legitimately run long must yield periodically. Code default is 0;
required (enforced) for Medical Class B/C and SIL 1+.

### 4.6 `OS_SAFETY_DEADLINE_ACTION` — Action on deadline miss
| | |
|---|---|
| **Default** | `1` |
| **Range** | 0 / 1 / 2 (compile-time checked) |
| **Safety** | [MED-B] [MED-C] [IND-2] [IND-3] |

- `0` = log only (insufficient for Class B/C and SIL 2+)
- `1` = reset the offending task (enables recovery)
- `2` = disable the task (enter safe state)

### 4.7 `OS_SAFETY_TASK_MAX_RECOVERY` — Max recovery attempts
| | |
|---|---|
| **Default** | `3` |
| **Range** | ≥ 0 |
| **Safety** | [MED-C] [IND-3] |

Max task recovery attempts before permanent disable. After repeated failures the system should
enter a safe state rather than recover indefinitely. `0` disables recovery (immediate safe state).

### 4.8 `OS_SAFETY_SOFT_WDG_TIMEOUT_MS` — Software watchdog timeout
| | |
|---|---|
| **Default** | `3000` ms |
| **Range** | > 0 |
| **Safety** | [MED-B] [MED-C] [IND-1] [IND-2] |

Must be ≤ the application's safety time / process safety time (IEC 62304 §5.4.6; IEC 61508 Part 1
§7.4.4 — diagnostic test interval). Common values: 500 ms (motor control), 3000 ms (slow process).

### 4.9 `OS_SAFETY_MAX_CRITICAL_US` — Max critical-section duration
| | |
|---|---|
| **Default** | `1000` µs |
| **Range** | > 0, `0` disables the check |
| **Safety** | [MED-C] [IND-3] |

Max duration allowed inside `OS_SAFE` before reporting `SAFE_TOO_LONG`. Long critical sections
increase interrupt latency (IEC 61508 Part 3 §7.4.13 — WCET/latency must be bounded and
documented).

---

## 5. Quick Reference — All Options at a Glance

| Constant | Default | Range | Purpose |
|---|---|---|---|
| `OS_KERNEL_TICK_PERIOD_US` | 100 | 100–1000 (÷1000) | Tick resolution (µs) |
| `OS_KERNEL_STACK_SIZE` | 512 | ≥ 64 | Default task stack (bytes) |
| `OS_KERNEL_MAX_PRIORITIES` | 32 | 2–256 | Scheduler priority slots |
| `OS_TOOL_EVENT` | 1 | 0/1 | Event signaling |
| `OS_TOOL_MUTEX` | 1 | 0/1 | Mutex with priority inheritance |
| `OS_TOOL_QUEUE` | 1 | 0/1 | Bounded FIFO queue |
| `OS_TOOL_SEMAPHORE` | 1 | 0/1 | Counting semaphore |
| `OS_TOOL_TICKLESS_IDLE` | 1 | 0/1 | Tickless idle (WFI) |
| `OS_MONITOR_DEADLINE` | 0 | 0/1 | Deadline monitoring |
| `OS_MONITOR_TCB_INTEGRITY` | 0 | 0/1 | TCB magic-number check |
| `OS_MONITOR_ERROR_LOG` | 1 | 0/1 | Error log ring buffer |
| `OS_MONITOR_ERROR_LOG_SIZE` | 32 | ≥ 1 | Error log capacity (entries) |
| `OS_SAFETY_RAM_TEST` | 0 | 0/1 | Background March C- RAM test |
| `OS_SAFETY_MPU` | 0 | 0/1 | Per-task MPU protection |
| `OS_SAFETY_HW_WATCHDOG` | 0 | 0/1 | IWDG feed/check integration |
| `OS_SAFETY_CRC_CHECK` | 0 | 0/1 | Flash CRC integrity check |
| `OS_SAFETY_SOFT_WATCHDOG` | 0 | 0/1 | Software stuck-task watchdog |
| `OS_SAFETY_DEADLINE_ACTION` | 1 | 0/1/2 | Deadline-miss reaction |
| `OS_SAFETY_TASK_MAX_RECOVERY` | 3 | ≥ 0 | Max recovery attempts |
| `OS_SAFETY_SOFT_WDG_TIMEOUT_MS` | 3000 | > 0 | Soft watchdog timeout (ms) |
| `OS_SAFETY_MAX_CRITICAL_US` | 1000 | > 0 | Max time inside OS_SAFE (µs) |

> ⚠️ **Differences from older docs:** the SAFETY_MANUAL's §6 tables historically listed
> `OS_MONITOR_DEADLINE`, `OS_MONITOR_TCB_INTEGRITY`, `OS_SAFETY_RAM_TEST`, `OS_SAFETY_MPU`,
> `OS_SAFETY_HW_WATCHDOG`, `OS_SAFETY_CRC_CHECK` and `OS_SAFETY_SOFT_WATCHDOG` as default 1.
> The **code defaults are 0** for those (see `ZenOS_Config.hpp`). This guide matches the code.

---

## 6. Ready-Made Configuration Profiles

### 6.1 Minimal (constrained devices, < 20 KB RAM)
```cpp
// ZenOS_Config.hpp — Minimal configuration
#define OS_KERNEL_TICK_PERIOD_US    1000UL  // 1ms tick (less overhead)
#define OS_KERNEL_STACK_SIZE        256     // smaller default stack

#define OS_TOOL_QUEUE               0
#define OS_TOOL_SEMAPHORE           0
#define OS_TOOL_TICKLESS_IDLE       0

#define OS_MONITOR_DEADLINE         0
#define OS_MONITOR_TCB_INTEGRITY    0
#define OS_MONITOR_ERROR_LOG        0

#define OS_SAFETY_RAM_TEST          0
#define OS_SAFETY_MPU               0
#define OS_SAFETY_HW_WATCHDOG       0
#define OS_SAFETY_CRC_CHECK         0
#define OS_SAFETY_SOFT_WATCHDOG     1       // keep — detects stuck tasks
```

### 6.2 Standard (recommended)
```cpp
// ZenOS_Config.hpp — Standard configuration: use the file as-is,
// everything sensible is enabled by default (see §5).
```

### 6.3 Safety-Target Profiles
Safety-target profiles are **not** edited in the config header — they are compiler flags that
enforce mandatory options at compile time:

```bash
# Medical device — IEC 62304 Class B / C
-DOS_TARGET_MEDICAL=2     # Class B
-DOS_TARGET_MEDICAL=3     # Class C (all safety features enforced)

# Industrial — IEC 61508 SIL 2 / SIL 3
-DOS_TARGET_INDUSTRIAL=2
-DOS_TARGET_INDUSTRIAL=3

# Combined — the stricter requirement applies
-DOS_TARGET_MEDICAL=3 -DOS_TARGET_INDUSTRIAL=3
```

Keep all code defaults enabled and add the flag; the build fails with `#error` if any mandatory
feature is disabled. See [§9](#9-compile-time-enforcement-of-safety-standards) for the exact
requirements per level. Medical Class C and SIL 3 additionally require
`OS_MONITOR_DEADLINE`, `OS_MONITOR_TCB_INTEGRITY`, `OS_SAFETY_RAM_TEST`, `OS_SAFETY_MPU`,
`OS_SAFETY_HW_WATCHDOG` and `OS_SAFETY_CRC_CHECK` to be **explicitly enabled** (see the notes in §3–§4).

---

## 7. How To Override Options

Three equivalent ways, in order of preference:

1. **Compiler flags (build system):**
   ```bash
   CFLAGS += -DOS_KERNEL_TICK_PERIOD_US=1000UL -DOS_SAFETY_MPU=1
   ```
   This survives library updates without touching the shipped header.

2. **Header edit:** uncomment/insert the `#define` before the option's `#ifndef` guard in
   `ZenOS_Config.hpp`. Keep project-specific overrides out of the distributed file when possible.

3. **Pre-include define:** define the option before including any ZenOS header; the `#ifndef`
   guards respect an existing definition.

Compile-time validation runs regardless of how you override: tick period must divide 1000,
`OS_KERNEL_MAX_PRIORITIES` must be 2–256, `OS_SAFETY_DEADLINE_ACTION` must be 0/1/2, and
`OS_TARGET_*` values must be in range — otherwise the build stops with `#error`.

## 8. Derived Constants (not user settings)

These are calculated in `ZenOS.hpp` — do not edit them:

| Macro | Definition | Purpose |
|---|---|---|
| `OS_MONITOR_ENABLED` | `1` if any of DEADLINE / TCB_INTEGRITY / ERROR_LOG | Gates the monitor API |
| `OS_TICKS_PER_MS` | `1000 / OS_KERNEL_TICK_PERIOD_US` | Tick↔ms conversion |
| `OS_PRIORITY_BITMAP_WORDS` | `ceil(OS_KERNEL_MAX_PRIORITIES / 32)` | Ready-bitmap words |
| `OS_PRIORITY_EXTRA_COUNT` | `MAX_PRIORITIES − 32` (if > 32) | Extension queue-head slots |
| `OS_PRIORITY_EXTRA_WORDS` | `ceil(OS_PRIORITY_EXTRA_COUNT / 32)` | Extension bitmap words |
| `OS_IDLE_STACK_WORDS` / `OS_FAULT_STACK_WORDS` | 64 / 48 | Idle & fault stack sizes |
| `OS_WAIT_FOREVER` | `0xFFFFFFFF` | Infinite-timeout sentinel |
| `OS_VERSION_*` | 1.0.1 | Version info reported by `os_get_version()` |

## 9. Compile-Time Enforcement of Safety Standards

Define `OS_TARGET_MEDICAL` (1 = Class A, 2 = B, 3 = C) and/or `OS_TARGET_INDUSTRIAL`
(1 = SIL 1 … 4 = SIL 4) via `-D` flags — **not** in `ZenOS_Config.hpp`. When set, the build fails
with `#error` if a mandatory option is disabled. Both can be combined; the stricter applies.

**IEC 62304 (Medical):**

| Config | Class A | Class B | Class C |
|--------|---------|---------|---------|
| `OS_MONITOR_DEADLINE` | — | ✅ REQUIRED | ✅ REQUIRED |
| `OS_MONITOR_ERROR_LOG` | — | ✅ REQUIRED | ✅ REQUIRED |
| `OS_SAFETY_SOFT_WATCHDOG` | — | ✅ REQUIRED | ✅ REQUIRED |
| `OS_SAFETY_DEADLINE_ACTION` | — | ≥ 1 REQUIRED | ≥ 1 REQUIRED |
| `OS_SAFETY_HW_WATCHDOG` | — | — | ✅ REQUIRED |
| `OS_MONITOR_TCB_INTEGRITY` | — | — | ✅ REQUIRED |
| `OS_SAFETY_MPU` | — | — | ✅ REQUIRED |
| `OS_SAFETY_CRC_CHECK` | — | — | ✅ REQUIRED |
| `OS_SAFETY_RAM_TEST` | — | — | ✅ REQUIRED |

**IEC 61508 (Industrial):**

| Config | SIL 1 | SIL 2 | SIL 3 |
|--------|-------|-------|-------|
| `OS_SAFETY_SOFT_WATCHDOG` | ✅ REQUIRED | ✅ REQUIRED | ✅ REQUIRED |
| `OS_MONITOR_ERROR_LOG` | ✅ REQUIRED | ✅ REQUIRED | ✅ REQUIRED |
| `OS_MONITOR_DEADLINE` | — | ✅ REQUIRED | ✅ REQUIRED |
| `OS_TOOL_MUTEX` | — | ✅ REQUIRED | ✅ REQUIRED |
| `OS_SAFETY_HW_WATCHDOG` | — | ✅ REQUIRED | ✅ REQUIRED |
| `OS_SAFETY_DEADLINE_ACTION` | — | ≥ 1 REQUIRED | ≥ 1 REQUIRED |
| `OS_SAFETY_MPU` | — | — | ✅ REQUIRED |
| `OS_SAFETY_RAM_TEST` | — | — | ✅ REQUIRED |
| `OS_SAFETY_CRC_CHECK` | — | — | ✅ REQUIRED |
| `OS_MONITOR_TCB_INTEGRITY` | — | — | ✅ REQUIRED |

> **⚠️ This is not certification.** The macros enforce *configuration completeness*, not compliance.
> The integrator must still perform the full safety lifecycle (risk analysis, V&V, documentation)
> per the applicable standard. Changing these macros constitutes a configuration change that must
> be documented (IEC 62304 §5.5).

---

*Generated for ZenOS RTOS v1.0.1 — the single source of truth for configuration is
[`ZenOS/ZenOS_Config.hpp`](../ZenOS/ZenOS_Config.hpp). See also: SAFETY_MANUAL.md, API_TUTORIAL.md.*
