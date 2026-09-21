<div align="center">
<img src="guide-html/ZenOS_logo.svg" alt="ZenOS Logo" width="400" />
</div>

# ZenOS RTOS — Configuration Guide

This document describes the configuration implemented by `ZenOS/ZenOS_Config.hpp`. The source header is the authoritative reference; the tables below describe the current configuration model directly.

## 1. Kernel

### `OS_KERNEL_TICK_PERIOD_US`

| Property | Value |
|---|---|
| Default | `100` µs |
| Valid range | `100..1000` µs |
| Constraint | Must divide `1000` evenly |

Controls the kernel tick period. `100` µs means a 10 kHz kernel tick.

### `OS_KERNEL_DEFAULT_STACK_SIZE`

| Property | Value |
|---|---|
| Default | `128` bytes |
| Minimum | `64` bytes (`static_assert` in the creation template) |

Default stack size used by task creation when no custom stack size is supplied. The creation template enforces an effective floor of `256` bytes (`StackBytesEff`): smaller requests still allocate a 256-byte buffer, because the kernel's stack initialization clamps stacks to at least 64 words (256 bytes). Actual stack sizing should be verified with the runtime stack report on the target build.

Per-task stacks can be supplied through `os_task_create_st(entry, prio, period, stack_bytes)`.

### `OS_KERNEL_MAX_PRIORITIES`

| Property | Value |
|---|---|
| Default | `16` slots |
| Valid range | `2..256` slots |
| Reserved priority | `0` (idle) |
| Usable application range | `1..OS_KERNEL_MAX_PRIORITIES-1` |
| Usable application range at 256 slots | **`1..255`** |

Priority convention (matches CMSIS-RTOS2, FreeRTOS, ThreadX): a **larger** number means a **higher** priority. `OS_KERNEL_MAX_PRIORITIES-1` is the highest usable priority; priority `1` is the lowest usable priority; priority `0` is the idle task and never runs while any user task is ready.

The scheduler keeps the original 32-bit ready bitmap for priorities `0..31` and derives extension storage for priorities above 31. The following values are derived automatically:

- `OS_PRIORITY_BITMAP_WORDS`
- `OS_PRIORITY_EXTRA_COUNT`
- `OS_PRIORITY_EXTRA_WORDS`

For `OS_KERNEL_MAX_PRIORITIES = 256`, the scheduler supports priorities `1..255` without a fixed 256-entry task table.

Approximate scheduler storage associated with priority queues and bitmap:

| Slots | Usable priorities | Queue-head storage | Bitmap storage |
|---:|---:|---:|---:|
| 32 | 1..31 | 128 B | 4 B |
| 64 | 1..63 | 256 B | 8 B |
| 128 | 1..127 | 512 B | 16 B |
| 256 | 1..255 | 1024 B | 32 B |

These figures describe scheduler priority storage, not the total RAM usage of an application.

### `OS_KERNEL_TICKLESS_IDLE_EN`

| Property | Value |
|---|---|
| Default | `1` (enabled) |

Tickless idle: when the idle task runs and tasks are blocked, the kernel programs SysTick for the next wakeup and advances `tick_count` by the skipped ticks on wake (`WFI`-based power saving). Disable for simpler, always-on tick timing.

### Idle and fault stacks (derived, overridable)

| Macro | Default | Meaning |
|---|---:|---|
| `OS_IDLE_STACK_WORDS` | 128 words (512 B) | Idle-task stack. The idle loop runs the HW-watchdog check, CRC step and tickless-idle path, and every interrupt taken while idle pushes an exception frame onto this stack. Override with `-DOS_IDLE_STACK_WORDS=n`. |
| `OS_FAULT_STACK_WORDS` | 64 words (256 B) | Fault-handler stack used to capture fault context and recover to the idle task. |

## 2. Scheduling and IPC

### `OS_IPC_TOOLS_EN`

| Property | Value |
|---|---|
| Default | `0` |
| Purpose | Single master switch for all inter-task communication primitives |

`OS_IPC_TOOLS_EN=1` enables **all** IPC together: `OS_EVENT`, `OS_MUTEX` (with immediate priority ceiling), `OS_QUEUE`, `OS_SEMAPHORE` and the `OS_LOCK`/`OS_LOCK_T` guards. With `OS_IPC_TOOLS_EN=1` the IPC classes are not compiled and the application must not use them. There are no separate per-primitive switches.

> Note for safety profiles: IEC 61508 SIL 2+ enforcement requires `OS_IPC_TOOLS_EN=1` (mutual exclusion for shared resources).

## 3. Runtime monitoring

| Option | Default | Purpose |
|---|---:|---|
| `OS_MONITORING_EN` | 0 | Master switch: deadline monitoring + TCB integrity checks + error log + CPU/stack instrumentation, enabled together |
| `OS_MONITORING_DEADLINE_ACTION` | 1 | Deadline-miss reaction (sub-option of `OS_MONITORING_EN`): 0 = log only, 1 = reset task, 2 = disable task; valid range 0..2 |
| `OS_MONITORING_ERROR_LOG_SIZE` | 16 | Error-log ring-buffer capacity in entries (sub-option of `OS_MONITORING_EN`); minimum 1 |

What `OS_MONITORING_EN=1` compiles in and runs (see the comment block in `ZenOS_Config.hpp`):

1. **Deadline monitoring** — `os_task_set_deadline()` arms a per-task hard deadline; `os_tick()` detects misses, counts them and reports `DEADLINE_MISS`; the reaction is configured by `OS_MONITORING_DEADLINE_ACTION`. `os_get_deadline_miss_count()` exposes the per-task miss count.
2. **TCB integrity checking** — every TCB carries a magic number (`OS_TCB_MAGIC`) written at task creation and verified before watchdog recovery actions; a corrupted TCB is reported as `TCB_CORRUPTED` and the affected task is deactivated instead of reset.
3. **Error log** — `OS_MONITORING_ERROR_LOG_SIZE` entries, each with timestamp, error code, task ID and severity. `_os_report_error()` logs every kernel error automatically. Query API: `os_log_error()`, `os_get_error_log_entry()`, `os_get_error_log_count()`, `os_get_error_log_total()`.
4. **CPU load and stack instrumentation** — `os_get_cpu_usage()`, `os_get_cpu_usage_total()`, `os_get_task_cpu_usage()`, per-task peak-SP watermark tracking, `os_get_stack_watermark()`, `os_get_stack_watermark_percent()` and the `os_get_stack_report()` table.

Setting `OS_MONITORING_EN=0` removes all of the above: the TCB loses those fields, the error-log API becomes no-op stubs and the monitor query functions are not compiled (guard application calls with `#if OS_MONITORING_EN`).

RAM cost when enabled (Cortex-M3, approximate): ~8 bytes per error-log entry (`OS_MONITORING_ERROR_LOG_SIZE` entries) + 16 bytes per task (deadline, magic and watermark fields).

## 4. Safety mechanisms

| Option | Default | Purpose |
|---|---:|---|
| `OS_SAFETY_RAM_TEST_EN` | 0 | Background March-C SRAM integrity test (incremental steps) |
| `OS_SAFETY_MPU_EN` | 0 | Per-task MPU protection on supported Cortex-M targets |
| `OS_SAFETY_HW_WATCHDOG_EN` | 0 | STM32 IWDG feed/check integration |
| `OS_SAFETY_CRC_EN` | 0 | Incremental flash integrity check through the CRC peripheral |
| `OS_SAFETY_SOFT_WATCHDOG_EN` | 0 | Stuck-task detection and recovery |
| `OS_SAFETY_TASK_MAX_RECOVERY` | 3 | Maximum recovery attempts per task before permanent disable (0 = immediate safe state) |
| `OS_SAFETY_SOFT_WDG_TIMEOUT_MS` | 3000 | Software watchdog timeout |
| `OS_SAFETY_MAX_CRITICAL_US` | 1000 | Maximum duration of an `OS_SAFE` section before `SAFE_TOO_LONG` (0 disables the check) |

The safety mechanisms are individually configurable. They are **not all enabled by default**; the configuration header is the source of truth for each default.

Hardware-availability guards (in `ZenOS_Port.hpp`) fail the build at compile time when a feature is requested on hardware that cannot support it:

- `OS_SAFETY_MPU_EN=1` on a target without MPU (`__MPU_PRESENT=0`, ARMv6-M) → `#error`
- `OS_SAFETY_CRC_EN=1` without a CRC peripheral (`CRC_BASE` undefined) → `#error`
- `OS_SAFETY_HW_WATCHDOG_EN=1` without IWDG (`IWDG_BASE` undefined) → `#error`

## 5. Safety target profiles

Define target profiles through compiler definitions, not by editing the target-profile section of the configuration header:

```text
-DOS_TARGET_MEDICAL_CLASS=1   # IEC 62304 Class A
-DOS_TARGET_MEDICAL_CLASS=2   # IEC 62304 Class B
-DOS_TARGET_MEDICAL_CLASS=3   # IEC 62304 Class C

-DOS_TARGET_INDUSTRIAL_SIL=1   # IEC 61508 SIL 1
-DOS_TARGET_INDUSTRIAL_SIL=2   # IEC 61508 SIL 2
-DOS_TARGET_INDUSTRIAL_SIL=3   # IEC 61508 SIL 3
-DOS_TARGET_INDUSTRIAL_SIL=4   # IEC 61508 SIL 4
```

When a target profile is active, the configuration header enforces the required ZenOS features with compile-time errors:

| Profile | Enforced at compile time |
|---|---|
| Medical Class B/C (`OS_TARGET_MEDICAL_CLASS >= 2`) | `OS_MONITORING_EN=1`, `OS_SAFETY_SOFT_WATCHDOG_EN=1`, `OS_MONITORING_DEADLINE_ACTION >= 1` |
| Medical Class C (`OS_TARGET_MEDICAL_CLASS >= 3`) | additionally `OS_SAFETY_HW_WATCHDOG_EN=1`, `OS_SAFETY_MPU_EN=1`, `OS_SAFETY_CRC_EN=1`, `OS_SAFETY_RAM_TEST_EN=1` |
| Industrial SIL 1+ (`OS_TARGET_INDUSTRIAL_SIL >= 1`) | `OS_SAFETY_SOFT_WATCHDOG_EN=1`, `OS_MONITORING_EN=1` |
| Industrial SIL 2+ (`OS_TARGET_INDUSTRIAL_SIL >= 2`) | additionally `OS_IPC_TOOLS_EN=1`, `OS_SAFETY_HW_WATCHDOG_EN=1`, `OS_MONITORING_DEADLINE_ACTION >= 1` |
| Industrial SIL 3+ (`OS_TARGET_INDUSTRIAL_SIL >= 3`) | additionally `OS_SAFETY_MPU_EN=1`, `OS_SAFETY_RAM_TEST_EN=1`, `OS_SAFETY_CRC_EN=1` |

When both profiles are defined, the stricter requirement applies. These checks validate configuration completeness; they do not constitute certification or product-level compliance.

## 6. Minimal configuration example

For a memory-constrained target, disable only features that the application does not need and verify the resulting RAM/flash footprint:

```cpp
#define OS_KERNEL_TICK_PERIOD_US  1000UL
#define OS_KERNEL_DEFAULT_STACK_SIZE  256

#define OS_IPC_TOOLS_EN              1   // 0 removes all IPC classes

#define OS_MONITORING_EN                1
#define OS_MONITORING_DEADLINE_ACTION         1
#define OS_MONITORING_ERROR_LOG_SIZE       16

#define OS_SAFETY_RAM_TEST_EN        0
#define OS_SAFETY_MPU_EN             0
#define OS_SAFETY_HW_WATCHDOG_EN     0
#define OS_SAFETY_CRC_EN             0
#define OS_SAFETY_SOFT_WATCHDOG_EN   0
```

The values above are an example profile, not a universal recommendation. Safety-targeted systems must enable the mechanisms required by their target profile.

## 7. Overriding configuration

Each configuration option is protected by `#ifndef`, so project-level definitions can be supplied before the header is processed. Compiler `-D` options are usually the cleanest way to keep project-specific configuration outside the distributed source.

Compile-time validation covers:

- `OS_KERNEL_TICK_PERIOD_US` (range 100..1000 and even-divisor rule)
- `OS_KERNEL_MAX_PRIORITIES` (2..256)
- `OS_MONITORING_DEADLINE_ACTION` (0..2)
- `OS_MONITORING_ERROR_LOG_SIZE` (>= 1)
- `OS_TARGET_MEDICAL_CLASS` / `OS_TARGET_INDUSTRIAL_SIL` ranges
- all target-profile feature enforcement listed in section 5

## 8. Derived values

The following are calculated automatically and must not be edited manually:

| Macro | Meaning | Defined in |
|---|---|---|
| `OS_PRIORITY_BITMAP_WORDS` | Ready bitmap words for all configured priorities | `ZenOS.hpp` |
| `OS_PRIORITY_EXTRA_COUNT` | Queue-head slots above priority 31 | `ZenOS.hpp` |
| `OS_PRIORITY_EXTRA_WORDS` | Bitmap words for the extension priorities | `ZenOS.hpp` |
| `OS_TICKS_PER_MS` | Tick conversion derived from `OS_KERNEL_TICK_PERIOD_US` | `ZenOS.hpp` |
| `OS_IDLE_STACK_SIZE` / `OS_FAULT_STACK_SIZE` | Byte sizes of the idle/fault stacks | `ZenOS.hpp` |
| `OS_WAIT_FOREVER` | Infinite-timeout constant | `ZenOS.hpp` |
| `OS_RAM_TEST_START` / `OS_RAM_TEST_END` | RAM test region (start = `__bss_end__` from the linker; end = lower half of RAM, overridable with `-DOS_RAM_TEST_END=<addr>`) | `ZenOS_Port.hpp` |
| `OS_HAS_MPU_HW` / `OS_HAS_FPU_HW` / `OS_HAS_CRC_HW` / `OS_HAS_IWDG_HW` / `OS_HAS_CYCLE_COUNTER` / `OS_HAS_VTOR` | Hardware capability detection (CMSIS device header + `__ARM_ARCH_*`) | `ZenOS_Port.hpp` |

## 9. Practical configuration advice

- Choose the number of priority slots based on the application. `256` slots are available when the full priority range `1..255` is required; the default is 16 slots to keep the baseline configuration small.
- Measure task stack peaks on the target compiler/optimization level before reducing `OS_KERNEL_DEFAULT_STACK_SIZE`.
- Disable `OS_IPC_TOOLS_EN` on tightly constrained products that use no inter-task communication.
- Enable the required safety mechanisms explicitly for safety-oriented projects and use the corresponding compile-time target profile.
- Treat target-specific features such as MPU, CRC and IWDG as hardware-dependent integrations — the port-level availability guards will reject impossible combinations.

---

**ZenOS RTOS v1.1.0 · MIT License · Raymon Research Team**
