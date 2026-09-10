<div align="center">
<img src="./ZenOS_logo.svg" alt="ZenOS Logo" width="400" />
</div>

# ZenOS RTOS — Safety Manual

This document describes the safety mechanisms, assumptions and integration responsibilities implemented by the current ZenOS source tree.

## 1. Safety position

ZenOS provides configurable mechanisms intended to support robust and safety-oriented embedded development. These mechanisms do **not** make ZenOS, an application, or a final product certified to IEC 62304, IEC 61508, ISO 26262, DO-178C, or another safety standard.

Product-level compliance remains the responsibility of the integrator, including hazard analysis, requirements, traceability, verification, validation, configuration control and evidence appropriate to the target domain.

## 2. Scheduler model

ZenOS is a preemptive priority-based RTOS for ARM Cortex-M.

- Priority `0` is reserved for the idle task.
- Application priorities are `1..255` when `OS_KERNEL_MAX_PRIORITIES=256`.
- The configuration range is `2..256` priority slots.
- The default is 32 slots, giving application priorities `1..31`.
- Each configured priority has its own ready queue.
- The scheduler uses a bitmap to locate active priority levels and checks task eligibility before selection.
- Periodic tasks use `period_ticks` and `next_run_time` as a release gate.
- PendSV is used for the Cortex-M context-switch path.

The scheduler therefore supports the full usable priority range without requiring a fixed 256-entry task table.

## 3. Static resource model

ZenOS uses statically sized task resources and does not use a general-purpose heap for its kernel task model.

The default task stack is `512` bytes and can be changed through `OS_KERNEL_STACK_SIZE` or supplied per task through `os_task_create_st()`.

Stack sizing must be measured on the actual target, compiler and optimization level. The runtime stack report should be used to establish a suitable margin.

## 4. Safety mechanisms

### 4.1 Stack protection

`OS_SAFETY_SOFT_WATCHDOG` controls software watchdog behavior, while stack canary and stack-pointer checks are used by the safety implementation to detect stack faults.

When a stack fault is detected, ZenOS can reset the task according to `OS_SAFETY_DEADLINE_ACTION` and `OS_SAFETY_TASK_MAX_RECOVERY` policy where applicable.

### 4.2 TCB integrity

`OS_MONITOR_TCB_INTEGRITY` enables a magic-number integrity check for task control blocks. Detected corruption is reported through the error system.

### 4.3 Deadline monitoring

`OS_MONITOR_DEADLINE` enables deadline tracking. Applications can assign a deadline with `os_task_set_deadline()` and inspect misses with `os_get_deadline_miss_count()`.

`OS_SAFETY_DEADLINE_ACTION` controls the reaction:

- `0` — log only
- `1` — reset the task
- `2` — disable the task

### 4.4 Software watchdog

`OS_SAFETY_SOFT_WATCHDOG` detects tasks that fail to yield or block within `OS_SAFETY_SOFT_WDG_TIMEOUT_MS`.

A legitimate long-running task must yield or block often enough to remain within the configured timeout.

### 4.5 Hardware watchdog

`OS_SAFETY_HW_WATCHDOG` integrates with the STM32 IWDG feed/check path. The IWDG peripheral itself must be configured for the target in CubeMX.

The watchdog timeout and feed policy must be designed against the application's actual safety time.

### 4.6 MPU

`OS_SAFETY_MPU` enables per-task memory protection on targets with suitable Cortex-M MPU hardware. MPU availability and region constraints are hardware-specific.

### 4.7 RAM test

`OS_SAFETY_RAM_TEST` provides incremental incremental SRAM integrity routine testing through `os_ram_test_step()`.

The test is intended for background execution. The integrator must account for the tested RAM range and for possible interaction with DMA or other software accessing the same memory.

### 4.8 CRC integrity

`OS_SAFETY_CRC_CHECK` provides incremental flash-integrity checking through the STM32 CRC peripheral. Flash contents must remain stable while a check is being performed.

### 4.9 Error logging

`OS_MONITOR_ERROR_LOG` enables a fixed-size RAM circular log. The capacity is controlled by `OS_MONITOR_ERROR_LOG_SIZE` and defaults to 32 entries.

The log is volatile and does not survive a reset unless the application persists relevant information elsewhere.

### 4.10 Critical-section diagnostics

`OS_SAFETY_MAX_CRITICAL_US` controls the diagnostic threshold for `OS_SAFE` sections. Long interrupt-disabled regions increase interrupt latency and should be avoided.

## 5. Current configuration defaults

The current source configuration uses the following defaults:

| Mechanism | Default |
|---|---:|
| `OS_MONITOR_DEADLINE` | 0 |
| `OS_MONITOR_TCB_INTEGRITY` | 0 |
| `OS_MONITOR_ERROR_LOG` | 1 |
| `OS_SAFETY_RAM_TEST` | 0 |
| `OS_SAFETY_MPU` | 0 |
| `OS_SAFETY_HW_WATCHDOG` | 0 |
| `OS_SAFETY_CRC_CHECK` | 0 |
| `OS_SAFETY_SOFT_WATCHDOG` | 0 |
| `OS_SAFETY_DEADLINE_ACTION` | 1 |
| `OS_SAFETY_TASK_MAX_RECOVERY` | 3 |
| `OS_SAFETY_SOFT_WDG_TIMEOUT_MS` | 3000 |
| `OS_SAFETY_MAX_CRITICAL_US` | 1000 |

Safety mechanisms are individually configurable. A project should enable only what its requirements and target hardware call for, while safety-target profiles enforce mandatory settings at compile time.

## 6. IEC target profiles

ZenOS provides compile-time configuration guards for:

- IEC 62304: `OS_TARGET_MEDICAL=1..3`
- IEC 61508: `OS_TARGET_INDUSTRIAL=1..4`

The guards reject configurations that omit required ZenOS mechanisms for the selected target profile.

They are configuration checks, not certification evidence by themselves.

## 7. Integration assumptions

- Create tasks before `os_start()`.
- Use ISR-safe IPC functions from interrupt handlers (`signal_from_isr()`, `put_from_isr()`, etc.).
- Keep `OS_SAFE` sections short and never perform blocking operations inside them.
- Set IPC mutex ceilings to the highest priority of tasks that use the mutex.
- Verify stack usage on the final target build.
- Configure hardware-dependent peripherals such as IWDG and CRC in the board project.
- Verify MPU capabilities and region limits before enabling `OS_SAFETY_MPU`.
- Define an application-level recovery and safe-state strategy for detected faults.

## 8. Important limitations

Safety mechanisms are diagnostic and fault-response building blocks. They cannot guarantee detection of every hardware or software failure mode.

Examples include:

- Stack checks are not a substitute for correct stack sizing.
- A TCB magic number cannot detect every corruption pattern.
- Deadline monitoring observes scheduler-visible execution behavior; it does not prove functional completion.
- RAM testing does not cover every possible memory fault and must be integrated carefully with DMA.
- CRC is an integrity check, not cryptographic authentication.
- MPU protection depends on hardware capabilities and correct region configuration.
- Error logs are RAM-resident unless the application persists them.
- A watchdog reset loses volatile state.

## 9. Error codes

The kernel currently defines error codes including `STACK_OVERFLOW`, `INVALID_EVENT_ID`, `TASK_AFTER_START`, `TASK_STUCK`, `SAFE_TOO_LONG`, `HARDFAULT`, `PRIORITY_CONFLICT`, `DEADLINE_MISS`, `TCB_CORRUPTED`, `SENSOR_TIMEOUT`, `SAFE_MUTEX_LOCK` and `RAM_TEST_FAIL`.

Use `os_log_error()` and the monitor API to collect application-specific evidence.

## 10. Integration checklist

Before shipping a safety-oriented application:

1. Freeze the exact ZenOS commit and compiler/toolchain versions.
2. Freeze `ZenOS_Config.hpp` settings and target-profile flags.
3. Measure every task's peak stack usage.
4. Verify timing and deadline behavior on the target hardware.
5. Exercise every configured fault-response path.
6. Verify watchdog, MPU, RAM-test and CRC behavior when enabled.
7. Define reset and safe-state behavior for the application.
8. Persist required diagnostic evidence outside the RAM-only error log.
9. Perform the complete product-level safety lifecycle required by the applicable standard.

---

**ZenOS RTOS v1.0.1 · MIT License · Raymon Research Team**
