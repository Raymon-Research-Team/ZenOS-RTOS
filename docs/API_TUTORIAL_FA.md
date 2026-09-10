<div align="center">
<img src="./ZenOS_logo.svg" alt="ZenOS Logo" width="400" />
</div>

# راهنمای API و مثال‌های ZenOS RTOS

این راهنما رفتار فعلی API عمومی ZenOS را توضیح می‌دهد.

## ۱. اولین برنامه

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

تسک‌ها را قبل از `os_start()` ایجاد کنید.

## ۲. تسک و اولویت

```cpp
os_task_create(task_main, 5);
os_task_create(task_sensor, 10, 100);
os_task_create_st(task_ui, 20, 0, 1024);
```

قواعد اولویت:

- `0` برای idle رزرو است.
- در پیکربندی ۲۵۶ اسلات، اولویت‌های کاربردی **`1..255`** هستند.
- عدد بزرگ‌تر یعنی اولویت بالاتر.
- تسک‌های آماده با اولویت برابر توسط scheduler چرخانده می‌شوند.
- mutex می‌تواند از طریق IPC ceiling اولویت فعال تسک را موقتاً افزایش دهد.

پیش‌فرض `OS_KERNEL_MAX_PRIORITIES=32` است؛ بنابراین محدوده پیش‌فرض کاربردی `1..31` است. هسته تا ۲۵۶ اسلات را پشتیبانی می‌کند.

## ۳. تسک دوره‌ای

پارامتر `period_ms` یک release interval واقعی در scheduler است. هسته `period_ticks` و `next_run_time` را نگهداری و پیش از انتخاب تسک eligibility را بررسی می‌کند.

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

`os_yield()` برای واگذاری CPU است؛ `os_delay_ms()` برای خواب و blocking با مدت مشخص استفاده می‌شود.

## ۴. زمان‌بندی

```cpp
os_delay_ms(100);
os_delay_us(10);
os_yield();

uint32_t tick = os_get_tick();
uint32_t ms   = os_get_ms();
uint32_t us   = os_get_us();
```

تیک پیش‌فرض ۱۰۰ میکروثانیه است و از `OS_KERNEL_TICK_PERIOD_US` قابل تنظیم است.

## ۵. بخش بحرانی

```cpp
volatile uint32_t counter = 0;

OS_SAFE {
    counter++;
}
```

`OS_SAFE` وقفه‌ها را برای مدت بلوک غیرفعال می‌کند. بخش بحرانی باید کوتاه باشد و نباید داخل آن عملیات blocking انجام شود.

## ۶. Event

```cpp
OS_EVENT data_ready;

if (data_ready.wait(1000)) {
    process_data();
}

data_ready.signal();
```

در ISR از نسخه امن استفاده کنید:

```cpp
data_ready.signal_from_isr();
```

Eventها شمارنده داخلی دارند. عملیات mask به محدوده mask سی‌ودوبیتی محدود است؛ IDهای بالاتر از ۳۱ باید به‌صورت تکی استفاده شوند.

## ۷. Mutex و IPC Ceiling

```cpp
OS_MUTEX uart_mutex(20);

OS_LOCK(uart_mutex) {
    send_message();
}
```

سقف mutex باید برابر بالاترین اولویت تسک‌هایی باشد که آن mutex را می‌گیرند.

## ۸. Queue

```cpp
OS_QUEUE<uint16_t, 8> sensor_queue;

sensor_queue.put(42);

uint16_t value;
if (sensor_queue.get(value, 1000)) {
    process(value);
}
```

صف‌ها ظرفیت compile-time دارند. در ISR از `put_from_isr()` استفاده کنید.

## ۹. Semaphore

```cpp
OS_SEMAPHORE buffers(3);

if (buffers.wait(1000)) {
    use_buffer();
    buffers.signal();
}
```

سمافورها برای pool منابع و هماهنگی بین ISR و task مناسب هستند و شمارنده آن‌ها محدود است.

## ۱۰. پایش

```cpp
uint32_t peak = os_get_stack_usage(task_sensor);
uint8_t cpu = os_get_task_cpu_usage(task_sensor);
uint8_t total = os_get_cpu_usage_total();
```

پایش deadline در صورت فعال‌سازی قابل استفاده است:

```cpp
#if OS_MONITOR_DEADLINE
os_task_set_deadline(task_sensor, 50);
uint32_t misses = os_get_deadline_miss_count(task_sensor);
#endif
```

## ۱۱. ثبت خطا

```cpp
os_log_error(OSError::SENSOR_TIMEOUT,
             (uint8_t)ErrorSeverity::WARNING);

uint32_t total = os_get_error_count();
uint32_t unexpected = os_get_unexpected_error_count();
OSError last = os_get_last_error();
```

در صورت فعال بودن `OS_MONITOR_ERROR_LOG`، خطاها در یک بافر حلقه‌ای RAM ذخیره می‌شوند.

## ۱۲. مکانیسم‌های ایمنی

مکانیسم‌های اصلی قابل تنظیم عبارت‌اند از:

- `OS_SAFETY_SOFT_WATCHDOG`
- `OS_SAFETY_HW_WATCHDOG`
- `OS_SAFETY_RAM_TEST`
- `OS_SAFETY_CRC_CHECK`
- `OS_SAFETY_MPU`
- `OS_MONITOR_TCB_INTEGRITY`
- `OS_MONITOR_DEADLINE`

مثال:

```cpp
#if OS_SAFETY_RAM_TEST
os_ram_test_step();
#endif

#if OS_SAFETY_CRC_CHECK
os_crc_check_step();
#endif
```

پروفایل‌های IEC 62304 و IEC 61508 بررسی‌های compile-time برای کامل‌بودن پیکربندی ZenOS هستند و به‌تنهایی گواهی محصول محسوب نمی‌شوند.

## ۱۳. مدل حافظه

منابع اصلی تسک‌ها به‌صورت ایستا مدیریت می‌شوند و مدل تسک‌های هسته از Heap عمومی استفاده نمی‌کند. اندازه پیش‌فرض پشته ۵۱۲ بایت است و با `os_task_create_st()` قابل تغییر است.

## ۱۴. الگوی pipeline

```cpp
OS_QUEUE<SensorData, 8> queue;

void task_acquire(void) {
    while (1) {
        queue.put(read_sensor(), 100);
        os_yield();
    }
}

void task_process(void) {
    while (1) {
        SensorData data;
        if (queue.get(data, 1000)) {
            process(data);
        }
    }
}
```

## ۱۵. مرجع پیکربندی

جزئیات همه گزینه‌ها در [CONFIG_GUIDE_FA.md](CONFIG_GUIDE_FA.md) و فایل مرجع [`ZenOS_Config.hpp`](../ZenOS/ZenOS_Config.hpp) قرار دارد.

---

**ZenOS RTOS v1.0.1 · ARM Cortex-M · C++11 · MIT License**
