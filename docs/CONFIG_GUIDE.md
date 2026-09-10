<div align="center">
<img src="guide-html/ZenOS_logo.svg" alt="ZenOS Logo" width="400" />
</div>

# ZenOS RTOS — Configuration Guide

This document describes the configuration that is implemented by `ZenOS/ZenOS_Config.hpp`. The source header is the authoritative reference; the tables below intentionally describe the current configuration model directly.

## 1. Kernel

### `OS_KERNEL_TICK_PERIOD_US`

| Property | Value |
|---|---|
| Default | `100` µs |
| Valid range | `100..1000` µs |
| Constraint | Must divide `1000` evenly |

Controls the kernel tick period. `100` µs means a 10 kHz kernel tick.

### `OS_KERNEL_STACK_SIZE`

| Property | Value |
|---|---|
| Default | `512` bytes |
| Minimum | `64` bytes |

Default stack size used by task creation when no custom stack size is supplied. Actual stack sizing should be verified with the runtime stack report on the target build.

### `OS_KERNEL_MAX_PRIORITIES`

| Property | Value |
|---|---|
| Default | `32` slots |
| Valid range | `2..256` slots |
| Reserved priority | `0` |
| Usable application range at 256 slots | **`1..255`** |

Priority values are real scheduler levels. There is no priority-band or `priority >> 3` mapping.

The scheduler keeps the original 32-bit ready bitmap for priorities `0..31` and derives extension storage for priorities above 31. The following values are derived automatically:

- `OS_PRIORITY_BITMAP_WORDS`
- `OS_PRIORITY_EXTRA_COUNT`
- `OS_PRIORITY_EXTRA_WORDS`

For `OS_KERNEL_MAX_PRIORITIES = 256`, the scheduler therefore supports priorities `1..255` without a fixed 256-entry task table.

Approximate scheduler storage associated with priority queues and bitmap:

| Slots | Usable priorities | Queue-head storage | Bitmap storage |
|---:|---:|---:|---:|
| 32 | 1..31 | 128 B | 4 B |
| 64 | 1..63 | 256 B | 8 B |
| 128 | 1..127 | 512 B | 16 B |
| 256 | 1..255 | 1024 B | 32 B |

These figures describe scheduler priority storage, not the total RAM usage of an application.

## 2. Scheduling and IPC

All `OS_TOOL_*` options use `1` for enabled and `0` for disabled.

| Option | Default | Purpose |
|---|---:|---|
| `OS_TOOL_EVENT` | 1 | Event signaling |
| `OS_TOOL_MUTEX` | 1 | Mutex and priority-ceiling synchronization |
| `OS_TOOL_QUEUE` | 1 | Bounded compile-time FIFO queues |
| `OS_TOOL_SEMAPHORE` | 1 | Counting/bounded semaphores |
| `OS_TOOL_TICKLESS_IDLE` | 1 | WFI-based idle behavior |

## 3. Runtime monitoring

| Option | Default | Purpose |
|---|---:|---|
| `OS_MONITOR_DEADLINE` | 0 | Deadline monitoring and configured deadline reaction |
| `OS_MONITOR_TCB_INTEGRITY` | 0 | TCB magic-number integrity checks |
| `OS_MONITOR_ERROR_LOG` | 1 | RAM-resident circular error log |
| `OS_MONITOR_ERROR_LOG_SIZE` | 32 | Number of error-log entries; minimum 1 |

`OS_MONITOR_ENABLED` is derived automatically from the monitor options.

## 4. Safety mechanisms

| Option | Default | Purpose |
|---|---:|---|
| `OS_SAFETY_RAM_TEST` | 0 | Incremental incremental SRAM integrity routine SRAM test |
| `OS_SAFETY_MPU` | 0 | Per-task MPU protection on supported Cortex-M targets |
| `OS_SAFETY_HW_WATCHDOG` | 0 | STM32 IWDG integration |
| `OS_SAFETY_CRC_CHECK` | 0 | Flash integrity check through CRC |
| `OS_SAFETY_SOFT_WATCHDOG` | 0 | Stuck-task detection and recovery |
| `OS_SAFETY_DEADLINE_ACTION` | 1 | `0=log`, `1=reset`, `2=disable` |
| `OS_SAFETY_TASK_MAX_RECOVERY` | 3 | Maximum task recovery attempts |
| `OS_SAFETY_SOFT_WDG_TIMEOUT_MS` | 3000 | Software watchdog timeout |
| `OS_SAFETY_MAX_CRITICAL_US` | 1000 | Maximum diagnostic duration for `OS_SAFE` |

The safety mechanisms are individually configurable. They are **not all enabled by default**; the configuration header is the source of truth for each default.

## 5. Safety target profiles

Define target profiles through compiler definitions, not by editing the target-profile section of the configuration header:

```text
-DOS_TARGET_MEDICAL=1   # IEC 62304 Class A
-DOS_TARGET_MEDICAL=2   # IEC 62304 Class B
-DOS_TARGET_MEDICAL=3   # IEC 62304 Class C

-DOS_TARGET_INDUSTRIAL=1   # IEC 61508 SIL 1
-DOS_TARGET_INDUSTRIAL=2   # IEC 61508 SIL 2
-DOS_TARGET_INDUSTRIAL=3   # IEC 61508 SIL 3
-DOS_TARGET_INDUSTRIAL=4   # IEC 61508 SIL 4
```

When a target profile is active, the configuration header enforces the required ZenOS features with compile-time errors.

These checks validate configuration completeness. They do not constitute certification or product-level compliance.

## 6. Minimal configuration example

For a memory-constrained target, disable only features that the application does not need and verify the resulting RAM/flash footprint:

```cpp
#define OS_KERNEL_TICK_PERIOD_US  1000UL
#define OS_KERNEL_STACK_SIZE      256

#define OS_TOOL_EVENT             1
#define OS_TOOL_MUTEX             1
#define OS_TOOL_QUEUE             0
#define OS_TOOL_SEMAPHORE         0
#define OS_TOOL_TICKLESS_IDLE     0

#define OS_MONITOR_DEADLINE       0
#define OS_MONITOR_TCB_INTEGRITY  0
#define OS_MONITOR_ERROR_LOG      0

#define OS_SAFETY_RAM_TEST        0
#define OS_SAFETY_MPU             0
#define OS_SAFETY_HW_WATCHDOG     0
#define OS_SAFETY_CRC_CHECK       0
#define OS_SAFETY_SOFT_WATCHDOG   0
```

The values above are an example profile, not a universal recommendation. Safety-targeted systems must enable the mechanisms required by their target profile.

## 7. Overriding configuration

Each configuration option is protected by `#ifndef`, so project-level definitions can be supplied before the header is processed. Compiler `-D` options are usually the cleanest way to keep project-specific configuration outside the distributed source.

Compile-time validation covers:

- `OS_KERNEL_TICK_PERIOD_US`
- `OS_KERNEL_MAX_PRIORITIES`
- `OS_SAFETY_DEADLINE_ACTION`
- medical and industrial target-profile ranges

## 8. Derived scheduler values

The following are calculated automatically in `ZenOS.hpp` and must not be edited manually:

| Macro | Meaning |
|---|---|
| `OS_PRIORITY_BITMAP_WORDS` | Ready bitmap words for all configured priorities |
| `OS_PRIORITY_EXTRA_COUNT` | Queue-head slots above priority 31 |
| `OS_PRIORITY_EXTRA_WORDS` | Bitmap words for the extension priorities |
| `OS_TICKS_PER_MS` | Tick conversion derived from `OS_KERNEL_TICK_PERIOD_US` |
| `OS_IDLE_STACK_WORDS` | Idle-task stack size in words |
| `OS_FAULT_STACK_WORDS` | Fault-task stack size in words |

## 9. Practical configuration advice

- Choose the number of priority slots based on the application. `256` slots are available when the full priority range `1..255` is required; the default remains 32 slots to keep the baseline configuration small.
- Measure task stack peaks on the target compiler/optimization level before reducing `OS_KERNEL_STACK_SIZE`.
- Disable unused IPC and monitoring features on tightly constrained products.
- Enable the required safety mechanisms explicitly for safety-oriented projects and use the corresponding compile-time target profile.
- Treat target-specific features such as MPU, CRC and IWDG as hardware-dependent integrations.

---

**ZenOS RTOS v1.0.1 · MIT License · Raymon Research Team**
