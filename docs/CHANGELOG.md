# Changelog — ZenOS RTOS

All notable changes to ZenOS are documented here.

Format follows [Keep a Changelog](https://keepachangelog.com/).

## [1.2.0] — 2026-09-16

### Removed
- **Debug subsystem (`os_debug`, `OS_DEBUG_ENABLED`, `dbg_*`):** The optional
  IPC debug-counter block and the `OS_DEBUG_ENABLED` configuration switch were
  removed entirely. They were compile-time dead by default (`OS_DEBUG_ENABLED=0`)
  and are not part of the production API. The IPC paths (`_os_mutex_block_on`,
  `_os_mutex_handoff`, `OS_MUTEX::lock`, `OS_MUTEX::unlock`) no longer carry any
  conditional counter code, so the mutex fast path is free of `#if` branches.
  The historical `ZenOS_Debug.hpp/.cpp` UART subsystem referenced by earlier
  changelog entries is not part of the current source tree and no longer appears
  in `ZenOS_Config.hpp`; the corresponding instructions were removed from
  `INTEGRATION_CUBEMX.md` and `PORTING_GUIDE.md`.

### Fixed (Critical — kernel)
- **Idle task stack overflow (silent memory corruption):** `OS_IDLE_STACK_WORDS`
  was 64 words (256 bytes). The idle loop is not a bare `wfi`: it calls
  `os_hw_watchdog_check()`, `os_crc_check_step()` and `os_idle_tickless()`
  (→ `_os_tickless_process()` → `_os_wake_task()` → `_os_pq_add()`), and any IRQ
  taken while idle adds a full exception frame on top. Measured peak at `-O0`
  was ~200 bytes (78 %), leaving almost no margin; overflow intermittently
  corrupted adjacent `.bss` and eventually produced a HardFault with
  `CFSR=0x00060500` and a corrupted return address. `_os_stack_check_all()`
  skips the idle task, so the overflow was never reported. Raised to 256 words
  (1 KB) and made overridable with `-DOS_IDLE_STACK_WORDS=n`. Verified: at 32
  words the fault reproduces deterministically; at 256 words the suite is stable
  across repeated runs and all optimization levels.
- **`_os_pq_remove()` cleared the ready bit for the wrong reason:**
  the function cleared a priority's ready bit whenever the removed task's
  predecessor link became null. When the removed task was the queue *tail*,
  `*pp` became null while the head and earlier tasks were still linked, so the
  bit was cleared with tasks still queued — permanently starving that priority
  level (the scheduler only scans priorities whose bit is set). Now the bit is
  cleared only when the priority's queue head is actually null.

### Fixed (Critical — sample application)
- **`HAL_IWDG_Refresh(&hiwdg)` called with an uninitialised handle:** the sample
  `MX_IWDG_Init()` had its body commented out, leaving `hiwdg.Instance == 0`.
  The first `HAL_IWDG_Refresh()` therefore wrote to address 0 and raised a
  BusFault (`CFSR=0x00040400`, IMPRECISERR+INVSTATE) that halted the test suite
  after the timing section. The IWDG is now initialised
  (`IWDG_PRESCALER_256`, reload 4095).

### Fixed (test harness — Core/Src/main.cpp)
- Helper tasks are created READY; they are now explicitly stopped after
  creation so they cannot run (and starve lower-priority tasks) before the test
  that owns them starts them.
- The priority-inversion helper tasks (`task_pi_low/med/high`) were rewritten to
  run one action per harness trigger and park by blocking, never by writing the
  shared `pi_mtx`/`pi_release_evt` pointers. Previously a task suspended inside
  `e->wait()` resumed a stale wait after `os_task_start()` and then called
  `unlock()` on a destroyed mutex, corrupting the following test.
- The `pi_high` ceiling test now also checks the holder's active priority while
  the high-priority task is blocked (it must equal the ceiling).

### Verified
- Full test suite passes at `-O0`, `-O1`, `-O2`, `-O3`, `-Os` and `-Og` on
  STM32F103C8T6 hardware (120 PASS / 0 FAIL / 1 interactive SKIP).
- Kernel translation units compile clean with `-Wall -Wextra` at every level.

## [1.1.0] — 2026-09-14

### Fixed (Bug #1 — Critical)
- **os_va_arg off-by-one in debug formatter (Bug #1):** `os_va_arg` macro in `ZenOS_Debug.cpp` advanced the pointer before reading, causing the first argument to be skipped. Fixed formula: `(*(type*)(((ap) += sizeof(type)) - sizeof(type)))`. Without this fix, `os_debug_printf()` and `os_debug_log()` always produced incorrect output.

### Fixed (Bug #2 — Major)
- **Format string length unbounded (Bug #2):** Added 128-byte cap to pointer-walk in both `os_debug_printf()` and `os_debug_log()` to match documented limit. Previously, a corrupted format string could cause an infinite loop or memory read fault.

### Fixed (Bug #3 — Minor)
- **Width field uncapped (Bug #3):** Added `width > 64` cap in the formatter to prevent excessive padding (local DoS) from malformed format strings.

### Fixed (Bug #4 — Major)
- **os_get_task_priority missing from C wrapper (Bug #4):** Added `os_get_task_priority()` declaration to `ZenOS_c.h` so C-only files can query task priority.

### Fixed (Bug #5 — Major)
- **os_task_exit did not remove task from priority queue (Bug #5):** `os_task_exit()` now calls `os_pq_remove(current_task)` before setting state to INACTIVE. Without this, the task remained in the scheduler queue as a ghost entry.

### Fixed (Bug #6 — Major)
- **os_init did not reset >32-priority extension storage (Bug #6):** `os_init()` now calls `os_priority_queues_init()` which properly initializes all bitmap and queue-head storage, including extension arrays for `OS_KERNEL_MAX_PRIORITIES > 32`.

### Fixed (Bug #7 — Medium)
- **os_yield used read-modify-write on ICSR (Bug #7):** Changed from `ldr/orr/str` to direct `STR 0x10000000` to `SCB->ICSR`, eliminating the race window where another ISR could set a different ICSR bit between read and write.

### Fixed (Bug #8 — Major)
- **Priority Inheritance broken with nested mutex locks (Bug #8):** Added `mutex_held_count` to TCB to track how many mutexs a task holds. `OS_MUTEX::unlock()` now only restores `base_priority` when `mutex_held_count` drops to zero, preventing premature priority drop when a task holds multiple mutexs with different ceilings.

### Fixed (Bug #9 — Medium)
- **os_dbg_flush() blocked indefinitely if UART TX stuck (Bug #9):** Added ~1ms timeout per byte in the ring-buffer flush path, matching the polling path's timeout behavior.

### Fixed (Bug #10 — Major)
- **Recovery counters inconsistent between soft watchdog and stack overflow (Bug #10):** Stack overflow recovery in `os_stack_check_all()` now uses the per-task `wdg_retries` counter (shared with soft watchdog), so both subsystems use a single unified recovery counter per task instead of two independent global counters.

### Added
- **SEGGER RTT backend (OS_DEBUG_BACKEND=1):** Compile-time selectable debug output via SWD/RTT instead of UART. No physical TX pin needed — ideal for industrial/medical environments where UART pins are unavailable. Zero overhead when `OS_DEBUG_BACKEND=0` (UART).
- **os_debug_init():** New function that initializes debug hardware (UART GPIO+USART or RTT buffer) at startup. Called automatically from `os_init()`.
- **Debug UART family mapping layer (ZenOS_Port.hpp):** Per-STM32-family default USART base, GPIO base, RCC enable register, and GPIO AF mechanism (F1 legacy vs modern MODER+AFR). Covers F0/F1/F2/F3/F4/F7/G0/G4/H7/L0/L1/L4/L5/U5/WB/WBA with correct register addresses from Reference Manuals.
- **os_fpu_is_active():** Helper function to detect FPU usage from exception context (checks EXC_RETURN bit 4).
- **RAM test override (OS_RAM_TEST_END):** `OS_RAM_TEST_END` is now overridable via project defines for full RAM coverage when needed.

### Changed
- **os_init() now calls os_priority_queues_init()** instead of manually clearing only the first 32 entries.
- **os_task_exit() now removes from priority queue** before setting INACTIVE state.
- **os_stack_check_all() uses unified per-task recovery** (task->wdg_retries) instead of global stack_recovery_count.
- **OS_MUTEX::lock/unlock track mutex_held_count** in TCB for correct priority restoration with nested locks.
- **OS_MUTEX::unlock() only restores base_priority** when no other mutexs are held (prevents priority drop while still holding another ceiling-boosted mutex).
- **ArchitectDocument.md version updated** to 2.0.0 (pre-existing mismatch fixed by ZenOS_Version.hpp in 1.0.2).

---

## [1.0.2] — 2026-09-13

### Fixed
- **Version mismatch (Critical):** Created `ZenOS_Version.hpp` as single source of truth for version numbers. All files (`ZenOS.hpp`, `ZenOS_Port.hpp`, `ZenOS_Safety.cpp`, etc.) now include this header instead of defining versions independently. Previously, `ZenOS_Port.hpp` and `ZenOS_Safety.cpp` declared v2.0.0 while `ZenOS.hpp` declared v1.0.1.
- **Wrong error code for MPU alignment fault (Critical):** In `ZenOS_Safety.cpp`, `os_mpu_set_region()` now reports `OSError::MPU_CONFIG_ERROR` instead of `OSError::PRIORITY_CONFLICT` when the MPU region base address is misaligned. A new `OSError::MPU_CONFIG_ERROR = 16` was added to the error enum.
- **CMSIS dependency in fallback detection (Critical):** In `ZenOS_Port.hpp`, the "unknown STM32" fallback and "Hardware Feature Overrides" sections no longer reference `__MPU_PRESENT`, `__FPU_PRESENT`, `__VTOR_PRESENT`, or `__NVIC_PRIO_BITS` from CMSIS headers. All feature detection now uses only `__ARM_ARCH_*` macros (set by `-mcpu` compiler flag) and STM32 family macros (set by CubeMX or project defines).
- **CMSIS DWT detection (Minor):** `OS_HAS_CYCLE_COUNTER` fallback now uses `__ARM_ARCH_*` instead of `defined(DWT)` from CMSIS.
- **Dead code round-robin bitmap (Major):** Removed unused `os_rr_skip`, `os_rr_is_skipped()`, `os_rr_set_skip()`, `os_rr_clear_all()`, and `os_ready_level_count()` from `ZenOS_Scheduler.cpp`. Round-robin fairness between same-priority tasks is already guaranteed by `os_pq_rotate()` (queue head rotation) + `next_run_time = tick_count + 1` (period gate). Added documentation explaining the fairness mechanism.
- **Tickless bug comment (Major):** Replaced the comment `"This is a bug we're fixing!"` in `os_tickless_process()` with a documented defensive assertion explaining why the `next_run_time` correction is necessary and that the path should never execute in correct code.
- **NVIC_PRIO_BITS fallback:** Simplified to always fall back to 4 for generic Cortex-M; removed conditional that referenced CMSIS.

### Added
- **`ZenOS_Version.hpp`:** Centralized version header with `OS_VERSION_MAJOR/MINOR/PATCH`, `OS_VERSION_PACKED`, and `OS_VERSION_STRING`.
- **`ZenOS_Regs.hpp`:** Consolidated register definitions for SCB, SysTick, DWT, MPU, IWDG, CRC peripherals. Replaces scattered raw address definitions across `ZenOS_Port.hpp` and `ZenOS_Safety.cpp`. All registers now use typed structs.
- **`OSError::MPU_CONFIG_ERROR = 16`:** New error code for MPU configuration failures (misaligned base address).
- **`ZenOS_Debug.hpp` / `ZenOS_Debug.cpp`:** Compile-time switchable UART debug subsystem with:
  - Boot banner (family, version, clock, architecture)
  - Context switch tracing (task name/ID before→after)
  - Fault dump (full register context)
  - Periodic statistics (CPU usage, stack watermarks)
  - Lightweight printf-like formatter (no stdio, no heap)
  - Compile-time log level filtering (ERROR/WARN/INFO/TRACE)
  - Optional interrupt-driven ring buffer mode
  - Zero overhead when `OS_DEBUG_UART=0`
- **`OS_DEBUG_UART` config option** in `ZenOS_Config.hpp` (default 0).
- **`PORTING_GUIDE.md`:** Step-by-step guide for adding new STM32 families.

### Changed
- All file `@version` annotations updated to 1.0.2.
- `ZenOS_Safety.cpp` fault handler now uses typed `OS_SCB->CFSR/HFSR/MMFAR/BFAR` instead of raw addresses.

---

## [1.0.1] — Previous Release

Initial stable release with:
- O(1) bitmap scheduler with round-robin
- Priority inheritance mutex (IPC ceiling)
- Bounded FIFO queue, counting semaphore, event system
- Stack canary (double-sided), fault handler with full context capture
- Hardware watchdog (IWDG), CRC integrity, RAM test (March-C)
- MPU protection (PMSAv7 and PMSAv8)
- Tickless idle with tick catch-up
- Multi-architecture support (ARMv6-M, ARMv7-M, ARMv7E-M, ARMv8-M)
- Comprehensive test suite (22+ test categories)
