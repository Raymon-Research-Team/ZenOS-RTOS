# ZenOS-RTOS Architecture Document

**Version:** 2.0.0  
**Date:** September 2026  
**Author:** Raymon Research Team

---

## Table of Contents

1. [Overview](#overview)
2. [Kernel Design](#kernel-design)
3. [File Structure](#file-structure)
4. [Task Management](#task-management)
5. [Scheduler](#scheduler)
6. [Context Switch](#context-switch)
7. [Interrupt Model](#interrupt-model)
8. [IPC Primitives](#ipc-primitives)
9. [Memory Safety](#memory-safety)
10. [MPU Protection](#mpu-protection)
11. [Fault Handling](#fault-handling)
12. [Hardware Abstraction](#hardware-abstraction)
13. [Timing Subsystem](#timing-subsystem)
14. [Safety Features](#safety-features)
15. [SMP Support](#smp-support)

---

## 1. Overview

ZenOS is a priority-based preemptive real-time operating system (RTOS) designed for ARM Cortex-M microcontrollers. It provides:

- **O(1) priority bitmap scheduler** with configurable priority levels (2–256)
- **FPU-aware context switch** for Cortex-M4F/M7
- **Hardware MPU integration** for memory protection (PMSAv7 and PMSAv8)
- **Immediate Priority Ceiling Protocol** for IPC (prevents unbounded priority inversion)
- **Complete safety instrumentation** — stack canaries, watchdog, CRC check, RAM test
- **IEC 62304 / IEC 61508 compliance support** with compile-time enforcement

### Design Principles

1. **Zero mock/fake implementations** — every API has a real implementation
2. **No API without behavior** — every function does what its signature promises
3. **No hidden crashes** — every fault path is handled gracefully
4. **Portable** — same kernel code runs on STM32F0 through STM32H7
5. **RAM-efficient** — no 256-entry static arrays; bitmap + linked list

---

## 2. Kernel Design

```
┌─────────────────────────────────────────────────┐
│                  Application                     │
│         (main.cpp — DO NOT MODIFY)               │
├─────────────────────────────────────────────────┤
│              Public API (ZenOS.hpp)              │
│  os_task_create / os_delay / OS_LOCK / etc.      │
├──────────┬──────────┬──────────┬────────────────┤
│ Scheduler│   IPC    │  Safety  │    Monitor     │
│ (.cpp)   │  (.cpp)  │  (.cpp)  │    (.cpp)     │
├──────────┴──────────┴──────────┴────────────────┤
│           Kernel Core (ZenOS.cpp)                │
│  init / start / PendSV / tick / delay / idle     │
├─────────────────────────────────────────────────┤
│     Internal API (ZenOS_Internal.hpp)            │
├─────────────────────────────────────────────────┤
│     Platform Port (ZenOS_Port.hpp)               │
│  STM32 family detection, register maps, arch     │
├─────────────────────────────────────────────────┤
│     Configuration (ZenOS_Config.hpp)             │
│  Feature switches, timing, safety levels         │
└─────────────────────────────────────────────────┘
```

### Compilation Units

| File | Role | Size (est.) |
|------|------|-------------|
| `ZenOS.cpp` | Kernel core: globals, init, start, PendSV, tick, idle, delay | ~3KB flash |
| `ZenOS_Scheduler.cpp` | O(1) bitmap scheduler, task create/control/lookup | ~2KB flash |
| `ZenOS_IPC.cpp` | Events, Mutex, C++ RAII wrappers | ~2KB flash |
| `ZenOS_Safety.cpp` | Error system, stack check, fault handler, watchdog, CRC, RAM test, MPU | ~2KB flash |
| `ZenOS_Monitor.cpp` | Stack watermark, CPU usage, deadline monitoring | ~1KB flash |

**Total estimated kernel size:** ~10KB flash, ~1.5KB RAM (default config, 10 tasks)

---

## 3. File Structure

```
ZenOS-RTOS/
├── ZenOS/
│   ├── ZenOS.hpp              # Public API (C and C++)
│   ├── ZenOS.cpp              # Kernel core
│   ├── ZenOS_Internal.hpp     # Internal shared declarations
│   ├── ZenOS_Config.hpp       # User configuration
│   ├── ZenOS_Port.hpp         # Platform-specific definitions
│   ├── ZenOS_c.h              # C-only header
│   ├── ZenOS_Scheduler.cpp    # Priority bitmap scheduler
│   ├── ZenOS_IPC.cpp          # Inter-Process Communication
│   ├── ZenOS_Safety.cpp       # Safety subsystem
│   └── ZenOS_Monitor.cpp      # Runtime monitoring
├── .github/workflows/
│   └── build.yml              # CI build matrix
├── docs/
│   ├── ARCHITECTURE.md        # This document
│   ├── PORTING_GUIDE.md       # How to port to new STM32 families
│   └── TEST_REPORT.md         # Test results
└── README.md
```

---

## 4. Task Management

### Task States

```
                 os_task_create()
                     │
                     ▼
    ┌──────────► INACTIVE ◄──────────────┐
    │              │                      │
    │         os_task_start()             │
    │              │                      │
    │              ▼                      │
    │           READY ◄───────────────┐   │
    │              │                  │   │
    │         PendSV selects         │   │
    │              │                  │   │
    │              ▼                  │   │
    │          RUNNING ──────────────┘   │
    │              │                  │   │
    │    os_delay / os_block   wake     │
    │              │                  │   │
    │              ▼                  │   │
    │          BLOCKED ──────────────┘   │
    │              │                      │
    │         os_task_stop()              │
    │         fault / stack overflow      │
    └────────────────────────────────────┘
```

### State Transition Rules

| From | Trigger | To | Validation |
|------|---------|----|-----------|
| INACTIVE | `os_task_start()` | READY | Task must exist, not already active |
| READY | PendSV selects | RUNNING | Only one task can be RUNNING |
| RUNNING | `os_delay()` / `os_block()` | BLOCKED | Must be in critical section |
| RUNNING | PendSV preempts | READY | State saved on stack |
| BLOCKED | Timeout / signal | READY | Wake task, increment blocked_count atomically |
| BLOCKED/READY/RUNNING | `os_task_stop()` | INACTIVE | Remove from PQ, decrement blocked_count if needed |
| BLOCKED/READY | Stack overflow | INACTIVE or reset | Max recovery attempts before permanent disable |
| Any | HardFault | INACTIVE | Task disabled, idle task recovers |

### Kernel Invariants

1. **Single RUNNING task** — exactly one task has `state == RUNNING` at any time
2. **No duplicate in PQ** — a task appears at most once in the priority queues
3. **No INACTIVE in PQ** — INACTIVE tasks are never in the priority queues
4. **blocked_count is accurate** — reflects the number of tasks in BLOCKED state
5. **task_count never decreases** — task IDs are monotonically increasing
6. **Deleted tasks are unreachable** — once INACTIVE, a task never executes code

### TCB Memory Layout

The TCB is layout-critical: assembly code depends on exact byte offsets.

```
Offset  Field           Size    Notes
──────  ──────────────  ──────  ──────────────────────
 0      stack_top       4B      Current PSP
 4      stack_base      4B      Bottom of stack buffer
 8      stack_size      4B      Stack size in words
12      state           1B      TaskState enum
13      priority        1B      Active priority (may be PI-boosted)
14      id              1B      Unique task ID
15      wait_result     1B      0=timeout, 1=success, 2=blocked-wake
16      name            4B      Debug name (const char*)
20      entry           4B      Task function pointer
24      period_ticks    4B      Periodic interval (0=aperiodic)
28      next_run_time   4B      Next allowed execution tick
32      delay_ticks     4B      Remaining delay
36      blocking_on     4B      Event/mutex causing block
40      block_timeout   4B      Remaining timeout
44      next            4B      Global task list linked list
48      last_yield_tick 4B      For watchdog/deadline
52      mutex_nesting   1B      Nested lock count
53      base_priority   1B      Original priority (before PI)
54      wdg_retries     1B      Watchdog recovery count
56      cpu_ticks       4B      CPU time accounting
60      queue_next      4B      Priority queue linked list
```

Assembly offsets are verified by `static_assert` at compile time.

---

## 5. Scheduler

### O(1) Priority Bitmap

The scheduler uses a **bitmap + linked list** design:

- `os_ready_bitmap` — 32-bit bitmask (priorities 0–31)
- `os_pq_head[32]` — linked list heads for priorities 0–31
- For >32 priorities: `os_ready_bitmap_ext[]` + `os_pq_head_ext[]`

**Selection:** `__builtin_clz()` finds the highest set bit in O(1).

### Priority Levels

- Priority 0: Reserved (idle task, never queued)
- Priorities 1–31: Primary range (no extra RAM)
- Priorities 32–255: Extended range (additional RAM)

### Round-Robin

Tasks at the same priority level are rotated on each PendSV tick:

```
os_pq_next()   → Select highest-priority eligible task
os_pq_rotate() → Move head to tail for round-robin fairness
```

### Periodic Tasks

Periodic tasks have `period_ticks > 0`. The scheduler enforces:

```
eligible = (period_ticks == 0) || ((int32_t)(now - next_run_time) >= 0)
```

This uses **signed comparison** for overflow safety:
- `(int32_t)(0xFFFFFFFF - 0x00000001) >= 0` → TRUE (correct: time has advanced)
- `(int32_t)(0x00000001 - 0xFFFFFFFF) < 0` → TRUE (correct: not yet time)

---

## 6. Context Switch

### PendSV Handler Flow

```
Exception Entry (hardware pushes R0-R3, R12, LR, PC, xPSR)
    │
    ├── If FPU was active (EXC_RETURN bit 4 = 0):
    │   Save S0-S15 + FPSCR (in exception frame, hardware)
    │   Save S16-S31 + FPSCR (in PendSV, software)
    │
    ├── Save R4-R11 (software)
    │
    ├── Update current_task->stack_top = PSP
    │
    ├── Set current_task state: RUNNING → READY
    │
    ├── Call os_pq_next() — select next task
    │   Call os_pq_rotate() — round-robin
    │
    ├── If next task uses FPU:
    │   Restore S16-S31 + FPSCR
    │
    ├── Restore R4-R11
    │
    └── Exception Return (hardware restores R0-R3, R12, LR, PC, xPSR)
```

### FPU Lazy Stacking

On Cortex-M4F/M7, the FPU registers are lazily stacked:
- If `CONTROL.FPCA` is set, the hardware stacks S0-S15 + FPSCR (72 bytes)
- PendSV additionally saves S16-S31 + FPSCR (68 bytes)
- Total FPU overhead: 140 bytes per FPU-using task

Non-FPU tasks have zero FPU overhead.

### Stack Frame Sizes

| Architecture | Exception Frame | Software Saved | Total |
|-------------|----------------|----------------|-------|
| Cortex-M0/M0+ | 8 words | 8 words | 16 words (64B) |
| Cortex-M3 | 8 words | 8 words | 16 words (64B) |
| Cortex-M4 (no FPU) | 8 words | 8 words | 16 words (64B) |
| Cortex-M4F (FPU active) | 26 words | 25 words | 51 words (204B) |
| Cortex-M7F (FPU active) | 26 words | 25 words | 51 words (204B) |

---

## 7. Interrupt Model

### Priority Allocation

| Handler | Priority | Rationale |
|---------|----------|-----------|
| SysTick | 0xFF (lowest) | Tick processing, never preempts anything |
| PendSV | 0xFE | Context switch, runs after all ISRs |
| Application ISRs | 0–5 | Hardware events (EXTI, UART, SPI, etc.) |

### Critical Sections

```c
uint32_t saved = os_critical_enter();  // CPSID I (disable IRQs)
// ... critical section ...
os_critical_exit(saved);                // Restore PRIMASK
```

**Rules:**
- Critical sections should be as short as possible (< 100µs default)
- ISR-level code should use `os_event_signal_from_isr()` / `os_queue.put_from_isr()`
- `OS_SAFE { ... }` is the C++ RAII wrapper — automatic entry/exit

---

## 8. IPC Primitives

### OS_MUTEX (Mutual Exclusion with Priority Ceiling)

```cpp
OS_MUTEX mtx(ceiling_priority);  // Create with ceiling
OS_LOCK(mtx) {                   // RAII lock guard
    // ... critical section ...
}                                 // Automatic unlock
```

**Immediate Priority Ceiling Protocol:**
- On `lock()`: task priority is boosted to `ceiling_priority`
- On `unlock()`: task priority is restored to `base_priority`
- Handoff: highest-priority waiter receives the mutex directly

### OS_EVENT (Inter-Task Signaling)

```cpp
OS_EVENT evt;
evt.signal();                    // Wake one waiter (or increment count)
int result = evt.wait(1000);     // Wait up to 1000ms, returns 1 on success
evt.signal_from_isr();           // ISR-safe signaling
```

### OS_SEMAPHORE (Counting Semaphore)

```cpp
OS_SEMAPHORE sem(5, 10);         // Initial=5, Max=10
sem.wait();                       // Decrement (block if zero)
sem.signal();                     // Increment (block if full)
sem.signal_from_isr();           // ISR-safe increment
```

### OS_QUEUE (Bounded FIFO)

```cpp
OS_QUEUE<int, 8> queue;           // Capacity=8, type=int
queue.put(42);                    // Block if full
queue.put_from_isr(42);          // Non-blocking ISR put
queue.get(item);                  // Block if empty
queue.get(item, 100);            // Timeout after 100ms
```

---

## 9. Memory Safety

### Stack Canary

Each task stack is initialized with:
- `OS_STACK_CANARY_COUNT` (8) words of `0xDEADBEEF` at the bottom
- `0xA5A5A5A5` fill pattern for the rest

**Detection:** `os_stack_check_all()` runs every 16 ticks and:
1. Checks if `stack_top` is below the canary region
2. Verifies canary words are intact

### Stack Watermark

```c
uint32_t used = os_get_stack_usage(task_fn);      // Bytes used
uint32_t pct  = os_get_stack_watermark_percent(id); // Percent used
```

Scans from bottom upward for the first non-`0xA5A5A5A5` word.

### RAM Test (March-C)

Non-destructive background test:
```
For each word:
  Read → Write ~Read → Read ~Read → Write Read (restore)
```

Runs one word per call from idle task.

---

## 10. MPU Protection

### PMSAv7 (Cortex-M3/M4/M7)

| Region | Base | Size | Permissions | Purpose |
|--------|------|------|-------------|---------|
| 0 | 0x08000000 | Flash size | RO, XN=0 | Code |
| 1 | 0x20000000 | RAM size | RW, XN=1 | Data |
| 2 | 0x40000000 | 512MB | RW, XN=1 | Peripherals |
| 3 | Task stack | Task stack size | RW, XN=1 | Per-task stack |
| 4+ | Configurable | Configurable | Configurable | User regions |

### PMSAv8 (Cortex-M23/M33)

Uses `RLAR` (Region Limit Address Register) instead of `RASR`. Supports up to 16 regions on Cortex-M33.

### Privilege Model

- **Kernel/ISRs:** Privileged (CONTROL.nPRIV=0) — MPU bypassed via PRIVDEFENA
- **Tasks:** Unprivileged (CONTROL.nPRIV=1) — MPU enforced

---

## 11. Fault Handling

### Fault Capture

On HardFault/MemManage/BusFault/UsageFault:

```
┌─────────────────────────────────────────┐
│ Fault Context (ZenOS_FaultContext)      │
├─────────────────────────────────────────┤
│ Exception Frame: R0, R1, R2, R3,       │
│   R12, LR, PC, xPSR                    │
│ NVIC: CFSR, HFSR, MMFAR, BFAR         │
│ Registers: CONTROL, MSP, PSP            │
│ Task: name, ID, stack_top, stack_size   │
└─────────────────────────────────────────┘
```

### Recovery

1. The offending task is disabled (`INACTIVE`)
2. The idle task is reset and scheduled
3. Fault context is stored in `os_last_fault` for post-mortem analysis
4. System continues operating — remaining tasks continue unaffected

### Error Codes

| Code | Name | Description |
|------|------|-------------|
| 0 | NONE | No error |
| 4 | STACK_OVERFLOW | Task stack overflow detected |
| 5 | INVALID_EVENT_ID | Signal/wait on unregistered event |
| 6 | TASK_AFTER_START | Attempt to create task after os_start() |
| 7 | TASK_STUCK | Soft watchdog timeout |
| 8 | SAFE_TOO_LONG | Critical section exceeded max duration |
| 9 | HARDFAULT | CPU fault (HardFault/MemManage/BusFault/UsageFault) |
| 10 | PRIORITY_CONFLICT | Scheduling priority issue |
| 11 | DEADLINE_MISS | Task missed its deadline |
| 12 | TCB_CORRUPTED | TCB magic number mismatch |
| 15 | RAM_TEST_FAIL | RAM integrity test failed |

---

## 12. Hardware Abstraction

### STM32 Family Support

ZenOS uses compile-time macros for hardware-specific details:

| Macro | Purpose | Varies by family |
|-------|---------|-----------------|
| `OS_VECTOR_COUNT` | Vector table size | 32–150 entries |
| `OS_CRC_BASE_ADDR` | CRC peripheral address | 0x40023000 (most) / 0x58024C00 (H7) |
| `OS_IWDG_BASE_ADDR` | IWDG peripheral address | 0x40003000 (most) / 0x58004800 (H7) |
| `OS_RCC_CSR_ADDR` | Reset control register | Family-specific |
| `OS_NVIC_PRIO_BITS` | Priority bits (2–4) | F0=2, most others=4 |
| `OS_HAS_FPU_HW` | Hardware FPU present | 0 for M0/M3, 1 for M4F/M7 |
| `OS_HAS_MPU_HW` | MPU present | 0 for M0/M0+, 1 for M3+ |
| `OS_HAS_CYCLE_COUNTER` | DWT->CYCCNT available | 0 for M0/M0+, 1 for M3+ |

### Supported STM32 Families

| Family | Core | Architecture | FPU | MPU | Status |
|--------|------|-------------|-----|-----|--------|
| STM32F0 | Cortex-M0 | ARMv6-M | ✗ | ✗ | ✓ Build |
| STM32F1 | Cortex-M3 | ARMv7-M | ✗ | ✓ | ✓ Build + Test |
| STM32F2 | Cortex-M3 | ARMv7-M | ✗ | ✓ | ✓ Build |
| STM32F3 | Cortex-M4 | ARMv7E-M | ✗* | ✓ | ✓ Build |
| STM32F4 | Cortex-M4F | ARMv7E-M | ✓ | ✓ | ✓ Build |
| STM32F7 | Cortex-M7F | ARMv7E-M | ✓ | ✓ | ✓ Build |
| STM32G0 | Cortex-M0+ | ARMv6-M | ✗ | ✗ | ✓ Build |
| STM32G4 | Cortex-M4F | ARMv7E-M | ✓ | ✓ | ✓ Build |
| STM32H7 | Cortex-M7F | ARMv7E-M | ✓ | ✓ | ✓ Build |
| STM32L0 | Cortex-M0+ | ARMv6-M | ✗ | ✓* | ✓ Build |
| STM32L1 | Cortex-M3 | ARMv7-M | ✗ | ✓ | ✓ Build |
| STM32L4 | Cortex-M4F | ARMv7E-M | ✓ | ✓ | ✓ Build |
| STM32L5 | Cortex-M33 | ARMv8-M | ✓ | ✓ | ✓ Build |
| STM32U5 | Cortex-M33 | ARMv8-M | ✓ | ✓ | ✓ Build |
| STM32WB | Cortex-M4F | ARMv7E-M | ✓ | ✓ | ✓ Build |
| STM32WBA | Cortex-M33 | ARMv8-M | ✓ | ✓ | ✓ Build |

*F303 has Cortex-M4 without FPU. L0 has optional MPU.

---

## 13. Timing Subsystem

### Tick Timer

SysTick configured for 100µs resolution (configurable):
- `OS_KERNEL_TICK_PERIOD_US = 100` (default)
- `OS_TICKS_PER_MS = 10` (derived)
- Maximum: 1000µs (1ms)

### Microsecond Timing (DWT)

Uses `DWT->CYCCNT` for cycle-accurate µs measurement:
- 32-bit counter wraps at ~59.6s (72MHz) or ~8.9s (480MHz)
- `os_time_fold()` accumulates wrap-safe µs via `cycle × 10^6 / SystemCoreClock`
- Fractional remainder prevents drift across fold periods

### Tickless Idle

When all tasks are blocked:
1. Calculate sleep duration (minimum remaining delay/timeout)
2. Program SysTick with appropriate reload value
3. Execute WFI (Wait For Interrupt)
4. On wake: calculate elapsed time and advance tick_count
5. Wake expired tasks and trigger PendSV

---

## 14. Safety Features

### IEC 62304 Compliance

| Feature | Class A | Class B | Class C |
|---------|---------|---------|---------|
| Deadline monitoring | Recommended | Required | Required |
| Error logging | Recommended | Required | Required |
| Soft watchdog | Recommended | Required | Required |
| HW watchdog | — | — | Required |
| TCB integrity | — | — | Required |
| MPU protection | — | — | Required |
| CRC check | — | — | Required |
| RAM test | — | — | Required |

### IEC 61508 Compliance

| Feature | SIL 1 | SIL 2 | SIL 3 |
|---------|-------|-------|-------|
| Soft watchdog | Required | Required | Required |
| Error logging | Required | Required | Required |
| Deadline monitoring | — | Required | Required |
| HW watchdog | — | Required | Required |
| MPU protection | — | — | Required |
| RAM test | — | — | Required |
| CRC check | — | — | Required |
| TCB integrity | — | — | Required |

---

## 15. SMP Support

> **Note:** SMP is currently marked as experimental (`OS_SMP_CORES > 1`).
> All production targets use single-core configurations.

When enabled:
- Task affinity: `os_task_set_core(entry, core)`
- Task migration: `os_task_migrate(entry, core)`
- Per-core idle tasks
- Inter-core interrupt (IPI) for cross-core scheduling

---

## Appendix A: Build Flags

```bash
# Minimal build (Cortex-M3, no FPU)
-mcpu=cortex-m3 -mthumb -DSTM32F1xx

# Full build (Cortex-M4F with FPU + MPU)
-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32F4xx

# Safety-critical build (IEC 62304 Class C)
-DOS_TARGET_MEDICAL=3 -DOS_SAFETY_MPU=1 -DOS_SAFETY_CRC_CHECK=1

# Minimal RAM build
-DOS_KERNEL_MAX_PRIORITIES=8 -DOS_KERNEL_STACK_SIZE=96 -DOS_MONITOR_ENABLED=0
```

## Appendix B: RAM Usage

| Component | Default Config | Minimal Config |
|-----------|---------------|----------------|
| Per-task TCB | 64 bytes | 64 bytes |
| Per-task stack | 512 bytes | 128 bytes |
| Priority bitmap (32) | 128 bytes | 128 bytes |
| Priority bitmap (256) | 1024 bytes | — |
| Idle task | 256 bytes | 256 bytes |
| Fault stack | 192 bytes | 192 bytes |
| Error log (32) | 128 bytes | — |
| RAM test state | 4 bytes | — |

---

*End of Architecture Document*
