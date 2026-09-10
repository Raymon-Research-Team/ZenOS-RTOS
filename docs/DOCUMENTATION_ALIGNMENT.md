# ZenOS Documentation Alignment

This note defines the current implementation facts that documentation must follow. It is intentionally additive and does not replace or remove the existing graphical, community, or donation content.

## Current implementation facts

- `OS_KERNEL_MAX_PRIORITIES` defaults to **32** and accepts **2..256** priority slots.
- Priority **0 is reserved for the idle task**. Application priorities are `1..OS_KERNEL_MAX_PRIORITIES-1`; with 256 slots they are `1..255`.
- A **larger numeric priority has higher scheduling priority**.
- The scheduler uses a **priority bitmap plus per-priority ready queues**. Configurations above 32 priorities use extension bitmap words and per-priority queue heads sized from the configured priority count; enabling 256 priorities does not create a fixed 256-task table.
- `OS_KERNEL_TICK_PERIOD_US` defaults to **100 µs** and is constrained to **100..1000 µs**, with values required to divide 1000 evenly.
- `OS_KERNEL_STACK_SIZE` defaults to **512 bytes** and `OS_KERNEL_MAX_TASKS` defaults to **16**.
- The kernel task model is **statically bounded**; it does not require a general-purpose heap for task/TCB allocation. `os_task_create_st()` permits an explicit stack size.
- Periodic tasks use a scheduler release gate based on their configured period.
- IPC includes events, mutexes with **Immediate Priority Ceiling (IPC)** support, compile-time bounded FIFO queues, and counting semaphores, with ISR-safe variants where provided.
- Runtime monitoring includes stack usage, CPU usage, deadline monitoring, and TCB integrity/error diagnostics.

## Safety configuration defaults

Safety and monitoring mechanisms are **configurable independently**; they are not all enabled by default. Current defaults include:

| Option | Default |
|---|---:|
| `OS_SAFETY_MPU` | `0` |
| `OS_SAFETY_RAM_TEST` | `0` |
| `OS_SAFETY_CRC_CHECK` | `0` |
| `OS_MONITOR_DEADLINE` | `0` |
| `OS_MONITOR_TCB_INTEGRITY` | `0` |
| `OS_MONITOR_ERROR_LOG` | `1` |
| `OS_SAFETY_SOFT_WATCHDOG` | `0` |
| `OS_SAFETY_HW_WATCHDOG` | `0` |

Applications should enable and validate the mechanisms required by their own safety analysis and target hardware.

## Safety-standard wording

ZenOS may provide **compile-time target-profile checks** associated with IEC 62304 and IEC 61508. These checks do **not** constitute certification of ZenOS or of an application. Certification, verification, validation, hazard analysis, and evidence remain the responsibility of the system integrator and the applicable assessment process.

## Documentation rule

When updating README, Markdown manuals, configuration guides, or HTML pages, preserve existing diagrams, images, styling, support/community information, and the DonatR section unless a separate request explicitly asks for their removal. Technical statements should be reconciled with the current implementation facts above.
