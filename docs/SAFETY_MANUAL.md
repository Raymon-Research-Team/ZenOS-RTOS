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

- Priority `0` is reserved for the idle task and is the **lowest** priority.
- A **larger** numeric value means a **higher** priority (CMSIS-RTOS2 / FreeRTOS / ThreadX convention).
- Application priorities are `1..255` when `OS_KERNEL_MAX_PRIORITIES=256`.
- The configuration range is `2..256` priority slots. The default is `16` slots, giving application priorities `1..15`.
- Each configured priority has its own ready queue; priorities `0..31` use the scalar bitmap storage and higher levels use derived extension storage.
- The scheduler uses a bitmap to locate active priority levels and checks task eligibility (release gate) before selection.
- Periodic tasks use `period_ticks` and `next_run_time` as a release gate; `os_pq_rotate()` plus the per-tick `next_run_time` gate provide round-robin fairness between same-priority tasks.
- PendSV is used for the Cortex-M context-switch path (FPU-aware variants for Cortex-M4F/M7).

The scheduler supports the full usable priority range without requiring a fixed 256-entry task table.

## 3. Static resource model

ZenOS uses statically sized task resources and does not use a general-purpose heap for its kernel task model.

The default task stack is `128` bytes (`OS_KERNEL_DEFAULT_STACK_SIZE`) and can be supplied per task through `os_task_create_st()`. The creation template enforces an effective floor of `256` bytes: the kernel's stack initialization fills at least 64 words (256 bytes), so requesting less still allocates 256 bytes and would otherwise corrupt adjacent memory.

Stack sizing must be measured on the actual target, compiler and optimization level. The runtime stack report should be used to establish a suitable margin.

## 4. Safety mechanisms

### 4.1 Stack protection

Stack canary words (`OS_STACK_CANARY`, 8 words at the bottom of every stack) and stack-pointer bound checks are part of the safety layer and are always active. `_os_stack_check_all()` runs every 16 ticks from the tick path and detects both real overflow (SP below the canary region) and external corruption (modified canary words).

On detection, the task is removed from the scheduler and either reset or disabled: recovery uses the per-task `wdg_retries` counter (shared with the soft watchdog) against `OS_SAFETY_TASK_MAX_RECOVERY`; after exhausting attempts, the task is permanently deactivated. With `OS_MONITORING_EN`, a corrupted TCB (`TCB_CORRUPTED`) is never reset — it is deactivated immediately.

### 4.2 TCB integrity

`OS_MONITORING_EN` enables a magic-number integrity check (`OS_TCB_MAGIC`) for task control blocks. Detected corruption is reported through the error system as `TCB_CORRUPTED`.

### 4.3 Deadline monitoring

`OS_MONITORING_EN` enables deadline tracking. Applications assign a deadline with `os_task_set_deadline()` (0 disables it) and inspect misses with `os_get_deadline_miss_count()`. Misses are detected in the tick path and recorded per task.

`OS_MONITORING_DEADLINE_ACTION` configures the enforced reaction:

- `0` — log only
- `1` — reset the task
- `2` — disable the task (safe state)

Values below `1` are rejected at compile time for IEC 62304 Class B/C and IEC 61508 SIL 2+ target profiles.

### 4.4 Software watchdog

`OS_SAFETY_SOFT_WATCHDOG_EN` detects tasks that fail to yield or block within `OS_SAFETY_SOFT_WDG_TIMEOUT_MS`. The check runs in the tick path every millisecond against the task's `last_yield_tick`.

A legitimate long-running task must yield or block often enough to remain within the configured timeout.

### 4.5 Hardware watchdog

`OS_SAFETY_HW_WATCHDOG_EN` integrates with the STM32 IWDG feed/check path. `os_hw_watchdog_check()` feeds only when the system is healthy (error count below a threshold and the scheduler is running); skipping the feed lets the IWDG reset the MCU. The IWDG peripheral itself must be configured for the target in CubeMX.

The watchdog timeout and feed policy must be designed against the application's actual safety time.

### 4.6 MPU

`OS_SAFETY_MPU_EN` enables per-task memory protection on targets with suitable Cortex-M MPU hardware (PMSAv7 on M3/M4/M7, PMSAv8 on M23/M33). Static regions (flash read-only, SRAM read/write, peripherals non-executable) plus a per-task stack region are programmed by the kernel; user tasks run unprivileged while the kernel and ISRs bypass the MPU through `PRIVDEFENA`. MPU availability and region constraints are hardware-specific and verified at compile time against the CMSIS device header.

### 4.7 RAM test

`OS_SAFETY_RAM_TEST_EN` provides an incremental March-C SRAM integrity test through `os_ram_test_step()` (one word per call: read → write complement → verify → restore).

The test region starts at `__bss_end__` (the linker symbol, so static data is never touched) and covers the lower half of RAM by default; the end address is overridable with `-DOS_RAM_TEST_END=<addr>`. The upper half typically holds the active stack and heap — a destructive test there would corrupt live data.

The test is intended for background execution from the idle task. The integrator must account for the tested RAM range and for possible interaction with DMA or other software accessing the same memory.

### 4.8 CRC integrity

`OS_SAFETY_CRC_EN` provides incremental flash-integrity checking through the STM32 CRC peripheral. `os_crc_init()` captures the expected CRC of the full flash image at boot; `os_crc_check_step()` (called from the idle task) re-computes 64 words per call and reports `HARDFAULT` on mismatch. Flash contents must remain stable while a check is being performed.

### 4.9 Error logging

`OS_MONITORING_EN` enables a fixed-size RAM circular log. The capacity is controlled by `OS_MONITORING_LOG_SIZE` (default 16 entries). Every kernel error reported through `_os_report_error()` is logged automatically with timestamp, error code, task ID and severity (critical codes map to `CRITICAL`, others to `WARNING`).

The log is volatile and does not survive a reset unless the application persists relevant information elsewhere.

### 4.10 Critical-section diagnostics

`OS_SAFETY_MAX_CRITICAL_US` controls the diagnostic threshold for `OS_SAFE` sections: when a guard holds interrupts off longer than the limit (measured with DWT CYCCNT where available), `SAFE_TOO_LONG` is reported. Setting it to `0` disables the check. Long interrupt-disabled regions increase interrupt latency and should be avoided.

## 5. Current configuration defaults

The current source configuration uses the following defaults:

| Mechanism | Default |
|---|---:|
| `OS_MONITORING_EN` | 1 |
| `OS_MONITORING_DEADLINE_ACTION` | 1 |
| `OS_MONITORING_LOG_SIZE` | 16 |
| `OS_IPC_TOOLS_EN` | 0 |
| `OS_SAFETY_RAM_TEST_EN` | 0 |
| `OS_SAFETY_MPU_EN` | 0 |
| `OS_SAFETY_HW_WATCHDOG_EN` | 0 |
| `OS_SAFETY_CRC_EN` | 0 |
| `OS_SAFETY_SOFT_WATCHDOG_EN` | 0 |
| `OS_SAFETY_TASK_MAX_RECOVERY` | 3 |
| `OS_SAFETY_SOFT_WDG_TIMEOUT_MS` | 3000 |
| `OS_SAFETY_MAX_CRITICAL_US` | 1000 |

Safety mechanisms are individually configurable. A project should enable only what its requirements and target hardware call for, while safety-target profiles enforce mandatory settings at compile time.

## 6. IEC target profiles

ZenOS provides compile-time configuration guards for:

- IEC 62304: `OS_TARGET_MEDICAL_CLASS=1..3` (Class A/B/C)
- IEC 61508: `OS_TARGET_INDUSTRIAL_SIL=1..4` (SIL 1..4)

The guards reject configurations that omit required ZenOS mechanisms for the selected target profile:

| Profile | Enforced |
|---|---|
| Class B/C | `OS_MONITORING_EN`, `OS_SAFETY_SOFT_WATCHDOG_EN`, `OS_MONITORING_DEADLINE_ACTION >= 1` |
| Class C | additionally `OS_SAFETY_HW_WATCHDOG_EN`, `OS_SAFETY_MPU_EN`, `OS_SAFETY_CRC_EN`, `OS_SAFETY_RAM_TEST_EN` |
| SIL 1+ | `OS_SAFETY_SOFT_WATCHDOG_EN`, `OS_MONITORING_EN` |
| SIL 2+ | additionally `OS_IPC_TOOLS_EN`, `OS_SAFETY_HW_WATCHDOG_EN`, `OS_MONITORING_DEADLINE_ACTION >= 1` |
| SIL 3+ | additionally `OS_SAFETY_MPU_EN`, `OS_SAFETY_RAM_TEST_EN`, `OS_SAFETY_CRC_EN` |

They are configuration checks, not certification evidence by themselves.

## 7. Integration assumptions

- Create tasks before `os_start()`.
- Use ISR-safe IPC functions from interrupt handlers (`signal_from_isr()`, `put_from_isr()`, `get_from_isr()`).
- Keep `OS_SAFE` sections short and never perform blocking operations inside them.
- Set IPC mutex ceilings to the highest priority of tasks that use the mutex.
- Verify stack usage on the final target build.
- Configure hardware-dependent peripherals such as IWDG and CRC in the board project.
- Verify MPU capabilities and region limits before enabling `OS_SAFETY_MPU_EN`.
- Define an application-level recovery and safe-state strategy for detected faults.

## 8. Important limitations

Safety mechanisms are diagnostic and fault-response building blocks. They cannot guarantee detection of every hardware or software failure mode.

Examples include:

- Stack checks are not a substitute for correct stack sizing.
- A TCB magic number cannot detect every corruption pattern.
- Deadline monitoring observes scheduler-visible execution behavior; it does not prove functional completion.
- RAM testing covers only the configured region (lower half of RAM by default) and must be integrated carefully with DMA.
- CRC is an integrity check, not cryptographic authentication.
- MPU protection depends on hardware capabilities and correct region configuration.
- Error logs are RAM-resident unless the application persists them.
- A watchdog reset loses volatile state.

## 9. Error codes

The kernel defines and reports the following error codes (`OSError` enum in `ZenOS.hpp`):

`SAFE_DELAY_MS`, `SAFE_YIELD`, `SAFE_EVENT_WAIT`, `STACK_OVERFLOW`, `INVALID_EVENT_ID`, `TASK_AFTER_START`, `TASK_STUCK`, `SAFE_TOO_LONG`, `HARDFAULT`, `DEADLINE_MISS`, `TCB_CORRUPTED`, `SAFE_MUTEX_LOCK`, `RAM_TEST_FAIL`, `MPU_CONFIG_ERROR`.

The enum also reserves `PRIORITY_CONFLICT` and `SENSOR_TIMEOUT` values for application-level use; they are not reported by the kernel itself.

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

**ZenOS RTOS v1.1.0 · MIT License · Raymon Research Team**
