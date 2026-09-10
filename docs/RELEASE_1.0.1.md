# ZenOS RTOS v1.0.1

## Release status

This document is the release baseline for ZenOS RTOS 1.0.1. It describes the implementation shipped with this release and deliberately avoids claims that are not established by the source tree.

## Highlights

- Configurable priority space from 2 to 256 slots, with priority 0 reserved for the idle task.
- Default priority configuration remains 32 slots.
- Bitmap-assisted ready-state tracking with per-priority ready queues.
- Static/bounded kernel task resources; the kernel task model does not require a general-purpose heap.
- Configurable 100–1000 µs kernel tick period; default is 100 µs.
- Periodic-task release eligibility.
- Events, mutexes with IPC priority ceiling, bounded FIFO queues, and counting semaphores.
- Stack, CPU, deadline, and diagnostic monitoring where enabled by configuration.
- Configurable MPU, RAM test, CRC, TCB-integrity, software-watchdog, and hardware-watchdog facilities.
- Rich HTML documentation, Support/DonatR content, and a current-implementation reference.

## Important qualification

ZenOS RTOS 1.0.1 is an open-source RTOS release. Configuration options named for safety or standards-related mechanisms do not by themselves constitute certification, compliance, or a safety certification claim. Actual behavior depends on the selected configuration, target port, compiler, linker script, and application integration.

## Version

The public API version is **1.0.1** (`OS_VERSION_MAJOR=1`, `OS_VERSION_MINOR=0`, `OS_VERSION_PATCH=1`).

## Release gate

The release is not considered final until source, optional-feature builds, tests, documentation, website content, version metadata, and CI have all passed the final audit.
