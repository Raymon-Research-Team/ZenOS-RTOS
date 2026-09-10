<div align="center">
<img src="./ZenOS_logo.svg" alt="ZenOS Logo" width="400" />
</div>

# Why ZenOS? — Technical Characteristics

ZenOS is designed as a compact ARM Cortex-M RTOS with a clear resource model, priority-based scheduling and configurable diagnostics. The comparison below focuses on characteristics that can be read directly from the current ZenOS implementation.

> Benchmark numbers depend on MCU, clock, compiler, optimization and measurement method. Do not treat unverified figures as universal performance claims.

## 1. Fine-grained kernel tick

ZenOS defaults to a `100 µs` kernel tick and allows configuration from `100` to `1000 µs`, subject to the compile-time divisor rule.

This is useful when the application needs sub-millisecond scheduler timing. It is still important to choose the tick according to the real timing requirements because a faster tick increases interrupt activity.

```cpp
#define OS_KERNEL_TICK_PERIOD_US 100UL
```

## 2. Priority-based scheduler with extended priority range

ZenOS uses a ready bitmap and per-priority queues. The scheduler checks active priority levels and task eligibility before selecting the next task.

The configuration supports:

- 32 priority slots by default
- up to 256 slots
- application priorities `1..255` at the maximum setting
- priority `0` reserved for idle
- independent ready queues for each configured priority

```cpp
#define OS_KERNEL_MAX_PRIORITIES 256
```

This gives the application 255 usable numeric priority levels without requiring a fixed 256-task table.

## 3. Static task resources

The kernel task model uses statically sized TCBs and stacks rather than a general-purpose heap. The default task stack is 512 bytes and can be overridden per task.

Benefits include:

- explicit RAM budgeting
- no general-purpose heap fragmentation in the task model
- compile-time queue capacities
- predictable resource ownership

## 4. C++11 API with bounded IPC

ZenOS uses C++11 templates and RAII for several interfaces:

```cpp
OS_QUEUE<uint16_t, 8> samples;
OS_MUTEX uart_mutex(20);
OS_SEMAPHORE buffers(3);
OS_EVENT data_ready;
```

The queue capacity is fixed at compile time. Mutex and critical-section helpers provide scoped resource management.

## 5. Periodic task release model

A task created with a non-zero `period_ms` has its period converted to ticks and stored in the TCB. The scheduler uses `next_run_time` as a release gate.

```cpp
void task_sensor(void) {
    while (1) {
        read_sensor();
        os_yield();
    }
}

os_task_create(task_sensor, 10, 100);
```

The distinction is intentional:

- `os_yield()` — give the scheduler a scheduling point
- `os_delay_ms()` — block the current task for a specified duration
- `os_delay_us()` — busy-wait for a short interval

## 6. Configurable safety mechanisms

The source tree provides independently configurable mechanisms including:

| Mechanism | Configuration |
|---|---|
| Stack protection | `OS_SAFETY_SOFT_WATCHDOG` and stack checks |
| TCB integrity | `OS_MONITOR_TCB_INTEGRITY` |
| Deadline monitoring | `OS_MONITOR_DEADLINE` |
| Error logging | `OS_MONITOR_ERROR_LOG` |
| Software watchdog | `OS_SAFETY_SOFT_WATCHDOG` |
| Hardware watchdog | `OS_SAFETY_HW_WATCHDOG` |
| MPU | `OS_SAFETY_MPU` |
| RAM test | `OS_SAFETY_RAM_TEST` |
| CRC | `OS_SAFETY_CRC_CHECK` |
| Critical-section diagnostic | `OS_SAFETY_MAX_CRITICAL_US` |

These are engineering mechanisms, not a claim of safety certification.

## 7. Resource-aware scaling

Increasing `OS_KERNEL_MAX_PRIORITIES` above 32 adds derived bitmap and queue-head storage only for the additional priority levels.

| Priority slots | Usable application priorities | Queue heads | Bitmap |
|---:|---:|---:|---:|
| 32 | 1..31 | 128 B | 4 B |
| 64 | 1..63 | 256 B | 8 B |
| 128 | 1..127 | 512 B | 16 B |
| 256 | 1..255 | 1024 B | 32 B |

This is why the full 1..255 priority range can be available without allocating a large task array.

## 8. Comparison in context

| Characteristic | ZenOS | Typical alternative RTOS design |
|---|---|---|
| Kernel tick | 100 µs default, configurable | Depends on port/configuration |
| Priority model | Bitmap + per-priority queues | Varies by scheduler |
| Maximum configurable priority slots | 256 | Varies by RTOS |
| Usable max priority in ZenOS | 255 | — |
| General-purpose heap in kernel task model | No | Often available |
| Queue capacity | Compile-time template | Commonly runtime/configuration based |
| Periodic task release | Scheduler release gate | Varies; often timer/delay based |
| C++ RAII API | Yes | Varies |
| Configurable safety mechanisms | Yes | Varies |
| IEC target checks | Compile-time configuration guards | Varies |

This table is intentionally conservative: RTOS behavior depends strongly on configuration and port, so individual features should be compared using the exact versions and configurations used by a project.

## 9. When ZenOS fits well

ZenOS is a good fit when you want:

- a small Cortex-M-oriented RTOS
- explicit static resource sizing
- a priority scheduler with up to 255 usable application priorities
- bounded IPC primitives
- C++11 APIs with RAII helpers
- configurable diagnostics and safety mechanisms
- compile-time configuration checks for selected IEC target profiles

For any production comparison, measure the exact workload on the target MCU rather than relying on generic benchmark numbers.

---

**ZenOS RTOS v1.0.1 · MIT License · Raymon Research Team**
