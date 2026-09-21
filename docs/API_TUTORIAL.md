<div align="center">
<img src="./ZenOS_logo.svg" alt="ZenOS Logo" width="400" />
</div>

# ZenOS RTOS — API Tutorial & Cookbook

ZenOS exposes a small C/C++ API for ARM Cortex-M systems. The examples below describe the current kernel behavior directly.

## 1. First application

```cpp
#include "ZenOS.hpp"

void task_blink(void) {
    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        os_delay_ms(500);
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    os_init();
    os_task_create(task_blink, 5);
    os_start();
}
```

Create tasks before `os_start()`. The scheduler takes control when `os_start()` is called; creating a task afterwards reports `TASK_AFTER_START` and fails.

## 2. Tasks and priorities

```cpp
os_task_create(task_main, 5);                 // default stack
os_task_create(task_sensor, 10, 100);         // priority 10, 100ms period
os_task_create_st(task_ui, 20, 0, 1024);      // custom 1024-byte stack
```

Priority rules:

- `0` is reserved for the idle task (the lowest priority); passing `0` is clamped to `1`.
- A larger numeric priority has higher scheduling priority.
- Application priorities are `1..OS_KERNEL_MAX_PRIORITIES-1` — `1..255` when configured to `256` slots (default is `16` slots, so `1..15`).
- Equal-priority ready tasks are rotated round-robin each tick by `os_pq_rotate()` plus the release gate.
- A mutex can temporarily raise a task's active priority through the IPC ceiling mechanism.
- Task stacks are at least 256 bytes (effective floor enforced by the creation template).

```cpp
uint8_t active  = os_get_task_priority(task_sensor);  // active (may be ceiling-boosted)
os_task_start(task_sensor);   // resume a stopped task, or (re)start an INACTIVE one
os_task_stop(task_sensor);    // non-destructive suspend: TCB/stack/wait preserved
bool active_now = os_task_isActive(task_sensor);
uint8_t state   = os_task_get_state(task_sensor);     // TaskState as uint8_t (0xFF if unknown)
```

`os_task_stop()`/`os_task_start()` match CMSIS-RTOS2 `osThreadSuspend`/`osThreadResume`: a task stopped while blocked on an event resumes waiting on that same event when started.

## 3. Periodic tasks

`period_ms` is a scheduler release interval. The scheduler tracks `period_ticks` and `next_run_time` and checks eligibility before selecting a task.

```cpp
void task_sensor(void) {
    while (1) {
        uint16_t value = read_sensor();
        sensor_queue.put(value, 50);
        os_yield();
    }
}

os_task_create(task_sensor, 10, 100);
```

Use `os_yield()` when the task has finished its current release. Use `os_delay_ms()` when you want the task to block for a specified duration.

## 4. Timing

```cpp
os_delay_ms(100);     // Blocking task delay (reports SAFE_DELAY_MS from ISR/OS_SAFE)
os_delay_us(10);      // Busy-wait; does not yield
os_yield();           // Explicit scheduling point

uint32_t tick = os_get_tick();
uint32_t ms   = os_get_ms();
uint32_t us   = os_get_us();   // DWT-based, wrap-safe accumulator
```

The default tick period is 100 µs and is configurable through `OS_KERNEL_TICK_PERIOD_US` (100..1000, must divide 1000 evenly).

`os_get_us()` uses a wrap-safe DWT cycle-counter extension; after changing `SystemCoreClock`, call `os_time_reset()` (or note that subsequent windows re-synchronize automatically).

## 5. Critical sections

```cpp
volatile uint32_t counter = 0;

OS_SAFE {
    counter++;
}
```

`OS_SAFE` disables interrupts for the guarded section and uses RAII to restore the previous PRIMASK state (nesting-safe via a depth counter). Keep these sections short. When DWT is available, the diagnostic limit `OS_SAFETY_MAX_CRITICAL_US` reports `SAFE_TOO_LONG` on overrun.

Do not call blocking APIs from `OS_SAFE` — `os_delay_ms()` and event waits with a timeout report `SAFE_DELAY_MS`/`SAFE_EVENT_WAIT` instead of blocking.

## 6. Events

Events require `OS_IPC_TOOLS_EN=1`.

```cpp
OS_EVENT data_ready;

void task_consumer(void) {
    while (1) {
        if (data_ready.wait(1000)) {
            process_data();
        }
    }
}

void task_producer(void) {
    while (1) {
        produce_data();
        data_ready.signal();
        os_yield();
    }
}
```

For interrupt handlers, use the ISR-safe operation:

```cpp
data_ready.signal_from_isr();
```

Event signaling is counter-based. IDs above 31 are supported individually; mask-based operations (`ev1 | ev2`, `signal_from_isr(mask)`) are limited to the 32-bit mask (IDs 0..31). Destroying an event wakes all tasks still blocked on it with a failure result.

## 7. Mutexes and IPC ceiling

```cpp
OS_MUTEX uart_mutex(20);   // ceiling = priority 20

OS_LOCK(uart_mutex) {      // RAII guard; OS_LOCK_T(mtx, timeout) for a timed variant
    HAL_UART_Transmit(&huart1, data, length, 100);
}
```

The ceiling should be the highest priority of any task that can acquire the mutex. On `lock()` the owner is immediately boosted to the ceiling; on the final `unlock()` the priority is restored to the base value (the `mutex_held_count` field tracks nested/multiple locks correctly). Unlocking hands the mutex directly to the highest-priority waiter.

Manual locking is also available:

```cpp
if (uart_mutex.lock(1000)) {
    send_message();
    uart_mutex.unlock();
}
```

`OS_MUTEX` is recursive: re-locking from the owner increments the nesting count. Destroying a mutex restores a boosted owner and wakes blocked waiters.

## 8. Queues

```cpp
OS_QUEUE<uint16_t, 8> sensor_queue;

sensor_queue.put(42);            // non-blocking (timeout 0)
sensor_queue.put(100, 100);      // absolute-deadline blocking put

uint16_t value;
if (sensor_queue.get(value, 1000)) {
    process(value);
}
```

Queues are bounded and compile-time sized. Timeout semantics use an absolute deadline: a contended `put(item, 100)` waits at most ~100 ms total, not 100 ms per retry loop iteration. ISR code should use the dedicated non-blocking operations:

```cpp
sensor_queue.put_from_isr(value);
sensor_queue.get_from_isr(value);
```

## 9. Semaphores

```cpp
OS_SEMAPHORE buffers(3);          // initial 3, unbounded max
OS_SEMAPHORE slots(0, 8);         // initial 0, max 8

if (buffers.wait(1000)) {
    use_buffer();
    buffers.signal();
}
```

Semaphores can be used for resource pools and ISR-to-task signaling. The maximum count is bounded; `signal_from_isr()` is available for interrupt context. Blocking `wait()` also uses the absolute-deadline rule.

## 10. Monitoring (OS_MONITORING_EN)

Monitoring requires `OS_MONITORING_EN=1`; guard calls with `#if OS_MONITORING_EN`:

```cpp
#if OS_MONITORING_EN
uint32_t peak = os_get_stack_usage(task_sensor);        // bytes used (watermark scan)
uint8_t  cpu  = os_get_task_cpu_usage(task_sensor);     // per-task CPU %
uint8_t  total = os_get_cpu_usage_total();               // system CPU %
uint8_t  win  = os_get_cpu_usage();                      // windowed CPU %

uint8_t n = os_get_stack_report_count();
for (uint8_t i = 0; i < n; i++) {
    os_stack_report_entry_t rep;
    os_get_stack_report(i, &rep);   // name, id, size_bytes, peak_bytes
}

uint32_t wm  = os_get_stack_watermark(task_id);          // by numeric task ID
uint32_t pct = os_get_stack_watermark_percent(task_id);
#endif
```

Deadline monitoring (part of `OS_MONITORING_EN`):

```cpp
#if OS_MONITORING_EN
os_task_set_deadline(task_sensor, 50);   // 50ms hard deadline; 0 disables
uint32_t misses = os_get_deadline_miss_count(task_sensor);
#endif
```

## 11. Error reporting

```cpp
os_log_error(OSError::SENSOR_TIMEOUT,
             (uint8_t)ErrorSeverity::WARNING);

uint32_t total      = os_get_error_count();
uint32_t unexpected = os_get_unexpected_error_count();
OSError  last       = os_get_last_error();
```

When `OS_MONITORING_EN` is enabled, errors are stored in a RAM circular buffer whose capacity is controlled by `OS_MONITORING_ERROR_LOG_SIZE`; `os_get_error_log_entry(i)`, `os_get_error_log_count()` and `os_get_error_log_total()` read it back. Errors are also logged automatically for every kernel-detected fault.

For deliberate fault injection (tests), wrap the region so expected errors do not count as unexpected:

```cpp
os_error_expect_begin();
    os_log_error(OSError::SENSOR_TIMEOUT, (uint8_t)ErrorSeverity::WARNING);
os_error_expect_end();
```

Kernel-detected error codes include `STACK_OVERFLOW`, `INVALID_EVENT_ID`, `TASK_AFTER_START`, `TASK_STUCK`, `SAFE_TOO_LONG`, `HARDFAULT`, `DEADLINE_MISS`, `TCB_CORRUPTED`, `SAFE_MUTEX_LOCK`, `RAM_TEST_FAIL` and `MPU_CONFIG_ERROR`. `PRIORITY_CONFLICT` and `SENSOR_TIMEOUT` are reserved for application-level use.

## 12. Safety APIs

The safety features are individually configurable:

- `OS_SAFETY_SOFT_WATCHDOG_EN`
- `OS_SAFETY_HW_WATCHDOG_EN`
- `OS_SAFETY_RAM_TEST_EN`
- `OS_SAFETY_CRC_EN`
- `OS_SAFETY_MPU_EN`

The query/feed functions are always declared and become no-op stubs when the feature is disabled:

```cpp
#if OS_SAFETY_RAM_TEST_EN
    os_ram_test_step();               // one word per call, from idle
    if (os_ram_test_complete() && os_get_ram_test_error_count() > 0) { /* SRAM fault */ }
#endif

#if OS_SAFETY_CRC_EN
    os_crc_check_step();              // os_crc_init() runs automatically in os_init()
    if (os_crc_check_complete() && os_get_crc_error_count() > 0) { /* flash changed */ }
#endif

#if OS_SAFETY_HW_WATCHDOG_EN
    os_hw_watchdog_feed();            // unconditional
    os_hw_watchdog_check();           // feed only when healthy
    uint32_t r = os_get_hw_wdg_reset_count();  // reads/clears the IWDG reset flag
#endif

#if OS_SAFETY_MPU_EN
    os_mpu_add_region(tcb_ptr, base, size, attrs);  // extra per-task region
#endif
```

Target profiles for IEC 62304 and IEC 61508 are compile-time configuration guards. They do not by themselves certify a product.

## 13. Platform and capability discovery

```cpp
printf("family: %s\n", os_get_family_name());   // e.g. "STM32F1"
printf("arch:   %s\n", os_get_arch_name());     // e.g. "ARMv7-M (Cortex-M3)"

uint32_t caps = os_get_capabilities();
if (caps & OS_CAP_MPU_ACTIVE) { /* tasks run under MPU */ }
if (caps & OS_CAP_CYCCNT)     { /* DWT microsecond timing available */ }
```

`os_get_capabilities()` returns a bitmask (`OS_CAP_MPU`, `OS_CAP_FPU`, `OS_CAP_MPU_ACTIVE`, `OS_CAP_CRC_HW`, `OS_CAP_IWDG_HW`, `OS_CAP_CYCCNT`, `OS_CAP_VTOR`, `OS_CAP_TICKLESS`) — the supported way to branch on target features instead of assuming an MCU name. Version: `os_get_version()` / `os_get_version_string()`.

## 14. Memory model

Task stacks and kernel control resources are statically sized. The default task stack is `128` bytes with a `256`-byte effective floor; custom stacks are supplied with `os_task_create_st()`. There is no general-purpose heap in the kernel task model.

Measure peak stack usage on the actual compiler configuration before choosing smaller stacks (`os_get_stack_usage()` / `os_get_stack_report()`).

## 15. Common patterns

### Sensor pipeline

```cpp
OS_QUEUE<SensorData, 8> raw_queue;

void task_acquire(void) {
    while (1) {
        raw_queue.put(read_sensor(), 100);
        os_yield();
    }
}

void task_process(void) {
    while (1) {
        SensorData data;
        if (raw_queue.get(data, 1000)) {
            process(data);
        }
    }
}
```

### Cooperative hand-off

```cpp
while (1) {
    do_small_unit_of_work();
    os_yield();
}
```

### Blocking worker

```cpp
while (1) {
    wait_for_event();
    do_work();
}
```

## 16. Build-time configuration

The principal configuration options are documented in [CONFIG_GUIDE.md](CONFIG_GUIDE.md). The source-of-truth header is [`ZenOS_Config.hpp`](../ZenOS/ZenOS_Config.hpp).

---

**ZenOS RTOS v1.1.0 · ARM Cortex-M · C++11 · MIT License**
