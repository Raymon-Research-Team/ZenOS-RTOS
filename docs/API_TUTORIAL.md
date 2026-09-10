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

Create tasks before `os_start()`. The scheduler takes control when `os_start()` is called.

## 2. Tasks and priorities

```cpp
os_task_create(task_main, 5);
os_task_create(task_sensor, 10, 100);
os_task_create_st(task_ui, 20, 0, 1024);
```

Priority rules:

- `0` is reserved for the idle task.
- Application priorities are `1..255` when `OS_KERNEL_MAX_PRIORITIES` is configured to `256`.
- A larger numeric priority has higher scheduling priority.
- Equal-priority ready tasks are rotated by the scheduler.
- A mutex can temporarily raise a task's active priority through the IPC ceiling mechanism.

The default configuration has 32 priority slots, so the default usable range is `1..31`. The kernel can be configured for up to 256 slots without creating a fixed 256-task table.

```cpp
uint8_t active = os_get_task_priority(task_sensor);
os_task_start(task_sensor);
os_task_stop(task_sensor);
bool active_now = os_task_isActive(task_sensor);
```

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
os_delay_ms(100);     // Blocking task delay
os_delay_ms(0);       // Yield-like immediate scheduling point
os_delay_us(10);      // Busy-wait; does not yield

uint32_t tick = os_get_tick();
uint32_t ms   = os_get_ms();
uint32_t us   = os_get_us();
```

The default tick period is 100 µs and is configurable through `OS_KERNEL_TICK_PERIOD_US`.

## 5. Critical sections

```cpp
volatile uint32_t counter = 0;

OS_SAFE {
    counter++;
}
```

`OS_SAFE` disables interrupts for the guarded section and uses RAII to restore the interrupt state. Keep these sections short. The diagnostic limit is controlled by `OS_SAFETY_MAX_CRITICAL_US`.

Do not call blocking APIs from `OS_SAFE`.

## 6. Events

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

Event signaling is counter-based. IDs above 31 are supported individually; mask-based operations are limited to the 32-bit mask.

## 7. Mutexes and IPC ceiling

```cpp
OS_MUTEX uart_mutex(20);

OS_LOCK(uart_mutex) {
    HAL_UART_Transmit(&huart1, data, length, 100);
}
```

The ceiling should be the highest priority of any task that can acquire the mutex. The active task priority is temporarily raised while the mutex is held.

Manual locking is also available:

```cpp
if (uart_mutex.lock(1000)) {
    send_message();
    uart_mutex.unlock();
}
```

## 8. Queues

```cpp
OS_QUEUE<uint16_t, 8> sensor_queue;

sensor_queue.put(42);
sensor_queue.put(100, 100);

uint16_t value;
if (sensor_queue.get(value, 1000)) {
    process(value);
}
```

Queues are bounded and compile-time sized. ISR code should use `put_from_isr()`.

```cpp
sensor_queue.put_from_isr(value);
```

## 9. Semaphores

```cpp
OS_SEMAPHORE buffers(3);

if (buffers.wait(1000)) {
    use_buffer();
    buffers.signal();
}
```

Semaphores can be used for resource pools and ISR-to-task signaling. The maximum count is bounded.

## 10. Monitoring

When monitoring is enabled, ZenOS provides stack and CPU information:

```cpp
uint32_t peak = os_get_stack_usage(task_sensor);
uint8_t cpu = os_get_task_cpu_usage(task_sensor);
uint8_t total = os_get_cpu_usage_total();
```

The stack report provides the peak and configured size for each task.

Deadline monitoring is optional:

```cpp
#if OS_MONITOR_DEADLINE
os_task_set_deadline(task_sensor, 50);
uint32_t misses = os_get_deadline_miss_count(task_sensor);
#endif
```

## 11. Error reporting

```cpp
os_log_error(OSError::SENSOR_TIMEOUT,
             (uint8_t)ErrorSeverity::WARNING);

uint32_t total = os_get_error_count();
uint32_t unexpected = os_get_unexpected_error_count();
OSError last = os_get_last_error();
```

When `OS_MONITOR_ERROR_LOG` is enabled, errors are stored in a RAM circular buffer whose capacity is controlled by `OS_MONITOR_ERROR_LOG_SIZE`.

## 12. Safety APIs

The following features are individually configurable:

- `OS_SAFETY_SOFT_WATCHDOG`
- `OS_SAFETY_HW_WATCHDOG`
- `OS_SAFETY_RAM_TEST`
- `OS_SAFETY_CRC_CHECK`
- `OS_SAFETY_MPU`
- `OS_MONITOR_TCB_INTEGRITY`
- `OS_MONITOR_DEADLINE`

Examples:

```cpp
#if OS_SAFETY_RAM_TEST
os_ram_test_step();
#endif

#if OS_SAFETY_CRC_CHECK
os_crc_check_step();
#endif
```

Target profiles for IEC 62304 and IEC 61508 are compile-time configuration guards. They do not by themselves certify a product.

## 13. Memory model

Task stacks and kernel control resources are statically sized. The default task stack is 512 bytes. Custom stacks can be supplied with `os_task_create_st()`.

Measure peak stack usage on the actual compiler configuration before choosing smaller stacks.

## 14. Common patterns

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

## 15. Build-time configuration

The principal configuration options are documented in [CONFIG_GUIDE.md](CONFIG_GUIDE.md). The source-of-truth header is [`ZenOS_Config.hpp`](../ZenOS/ZenOS_Config.hpp).

---

**ZenOS RTOS v1.0.1 · ARM Cortex-M · C++11 · MIT License**
