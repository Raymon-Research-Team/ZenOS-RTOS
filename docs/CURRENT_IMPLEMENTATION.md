# ZenOS — Current Implementation Reference

> This page is the technical source of truth for documentation claims about the current ZenOS kernel. It describes the implementation currently present in the repository; it does not imply product certification.

## Scheduler and priorities

- `OS_KERNEL_MAX_PRIORITIES` defaults to **32** and accepts **2..256** priority slots.
- Priority `0` is reserved for the idle task; application priorities are `1..OS_KERNEL_MAX_PRIORITIES-1`.
- A larger numeric priority has higher scheduling priority.
- The scheduler uses a priority bitmap plus per-priority ready queues and checks periodic-release eligibility before selection.
- For configurations above 32 slots, extension bitmap words and ready-queue heads are derived from the configured priority count; ZenOS does not allocate a fixed 256-entry task table merely because 256 priority slots are supported.
- Scheduler selection is **bitmap-assisted and priority-bounded**; it should not be described as strict O(1) independent of configured priority count because eligibility and queue traversal can require additional work.

## Timing

- `OS_KERNEL_TICK_PERIOD_US` defaults to **100 µs**.
- The configured tick period is constrained to **100–1000 µs** and must divide 1000 evenly.
- `os_delay_ms()` converts milliseconds to scheduler ticks.
- `os_delay_us()` is intended for short sub-tick delays and uses target cycle-counter support where available.

## Tasks and memory

- `OS_KERNEL_STACK_SIZE` defaults to **512 bytes**.
- There is **no fixed `OS_KERNEL_MAX_TASKS=16` limit in the current task-creation implementation**; task storage is supplied statically by each task-creation template instance.
- Task/TCB resources are statically sized; ZenOS does not require a general-purpose heap for its kernel task model.
- `os_task_create_st()` allows an explicitly supplied stack size.
- Application priority `0` is invalid/reserved; the idle task owns priority `0`.

## Periodic execution

- `os_task_create()` accepts `period_ms`, whose default is `0` (non-periodic).
- Periodic tasks are subject to a scheduler release gate: when the next release time has not arrived, the task is not eligible to run.
- After an eligible periodic execution, the next release time is advanced by the configured period.

## IPC

ZenOS provides configurable IPC mechanisms including:

- events
- mutexes with IPC priority-ceiling support
- compile-time bounded FIFO queues
- counting semaphores
- ISR-safe variants where provided by the API

Do not describe ZenOS mutexes as generic priority inheritance; the current implementation exposes an IPC priority-ceiling mechanism.

## Monitoring and diagnostics

The kernel includes configurable monitoring for stack usage, CPU usage, deadline behavior, TCB integrity and error logging. Error logging is enabled by default; other monitoring features may be disabled by default.

## Safety mechanisms

Safety-related mechanisms are **configuration-dependent**, not universally enabled by default. Current configuration defaults include:

| Mechanism | Current default |
|---|---:|
| MPU | `0` |
| RAM test | `0` |
| CRC check | `0` |
| Deadline monitoring | `0` |
| TCB integrity monitoring | `0` |
| Error log | `1` |
| Software watchdog | `0` |
| Hardware watchdog | `0` |
| Deadline action | `1` |

The RAM test is an incremental SRAM integrity routine implemented in the current source; documentation should not call it a full normative March C- implementation without a separate verification basis.

ZenOS also contains compile-time target-profile checks for IEC 62304- and IEC 61508-oriented configuration requirements. These checks do **not** constitute certification of the OS, application, firmware, or final product.

## Release 1.0.1 documentation rule

Historical claims must not override the implementation described here. In particular, documentation must not claim that all safety mechanisms are enabled by default, that priority `0` is an application priority, that only 32 priorities are supported, that the scheduler is strict O(1), that there is a fixed 16-task kernel table, or that ZenOS itself is IEC-certified.
