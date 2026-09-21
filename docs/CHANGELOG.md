# Changelog — ZenOS RTOS

All notable changes to ZenOS are documented here.

Format follows [Keep a Changelog](https://keepachangelog.com/).

## [1.1.0] — Current source version

### Added
- Priority bitmap and per-priority ready queues with configurable `OS_KERNEL_MAX_PRIORITIES` from 2 to 256 slots.
- Scheduler eligibility/release gating for periodic tasks through `period_ticks` and `next_run_time`.
- Non-destructive task stop/start semantics with `SUSPENDED` task state.
- C and C++ public API coverage through `ZenOS_c.h` and `ZenOS.hpp`.
- Platform/capability discovery through `os_get_family_name()`, `os_get_arch_name()` and `os_get_capabilities()`.
- Compile-time hardware capability checks for MPU, CRC and IWDG availability.
- Configurable tickless idle through `OS_KERNEL_TICKLESS_IDLE_EN`.
- Configurable monitoring, watchdog, MPU, CRC and RAM-test mechanisms.
- Compile-time IEC 62304 medical and IEC 61508 industrial target-profile configuration checks.

### Changed
- Kernel timing defaults to a `100 µs` tick; `OS_KERNEL_TICK_PERIOD_US` accepts `100..1000 µs` values that divide 1000 evenly.
- Default scheduler configuration uses 16 priority slots, with priority `0` reserved for idle and higher numeric values representing higher priority.
- IPC is controlled by the unified `OS_IPC_TOOLS_EN` switch and is enabled by default.
- Runtime monitoring is controlled by `OS_MONITORING_EN` and is disabled by default.
- Error-log capacity is configured by `OS_MONITORING_ERROR_LOG_SIZE`.
- Safety mechanisms remain independently configurable rather than being universally enabled.
- Documentation now describes target-profile checks as configuration enforcement, not as safety certification.

### Safety and portability
- MPU configuration is accepted only when the selected target reports compatible MPU hardware.
- CRC and IWDG safety features are rejected at compile time when the corresponding peripheral is not detected.
- Cortex-M architecture and STM32 family information is derived from compiler/device definitions in `ZenOS_Port.hpp`.
- Generic Cortex-M fallback detection remains available when no supported STM32 family macro is present.

### Documentation
- API, configuration, architecture, safety, CubeMX integration, porting and technical-characteristics documentation are synchronized with the current 1.1.0 source tree.
- English and Persian documentation use the same configuration names and current defaults.
- The interactive HTML guide remains visually unchanged while its factual configuration content is kept aligned with the source.

> Version `1.1.0` is the source-defined ZenOS version in `ZenOS/ZenOS_Version.hpp`.
> Product-level IEC certification or compliance is not implied by ZenOS target-profile checks.

---

**ZenOS RTOS v1.1.0 · MIT License · Raymon Research Team**