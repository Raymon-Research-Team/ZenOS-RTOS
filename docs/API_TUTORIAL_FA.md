<div align="center">
<img src="./ZenOS_logo.svg" alt="ZenOS Logo" width="400" />
</div>

<div dir="rtl">

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

تسک‌ها را قبل از `os_start()` ایجاد کنید. با فراخوانی `os_start()` زمان‌بند کنترل را می‌گیرد؛ ایجاد تسک بعد از آن کد `TASK_AFTER_START` را گزارش کرده و شکست می‌خورد.

## ۲. تسک و اولویت

```cpp
os_task_create(task_main, 5);                 // پشته پیش‌فرض
os_task_create(task_sensor, 10, 100);         // اولویت ۱۰، دوره ۱۰۰ms
os_task_create_st(task_ui, 20, 0, 1024);      // پشته سفارشی ۱۰۲۴ بایتی
```

قواعد اولویت:

- `0` برای idle رزرو است (پایین‌ترین اولویت)؛ مقدار ۰ به ۱ محدود می‌شود.
- **عدد بزرگ‌تر یعنی اولویت بالاتر.**
- اولویت‌های کاربردی برنامه `1..OS_KERNEL_MAX_PRIORITIES-1` هستند — با ۲۵۶ اسلات یعنی `1..255` (پیش‌فرض ۱۶ اسلات است، یعنی `1..15`).
- تسک‌های آماده با اولویت برابر در هر تیک توسط زمان‌بند round-robin چرخانده می‌شوند.
- mutex می‌تواند از طریق IPC ceiling اولویت فعال تسک را موقتاً افزایش دهد.
- پشته تسک‌ها حداقل ۲۵۶ بایت است (کف مؤثر در الگوی ایجاد اعمال می‌شود).

```cpp
uint8_t active  = os_get_task_priority(task_sensor);  // اولویت فعال (ممکن است سقف‌خورده باشد)
os_task_start(task_sensor);   // ادامه تسک متوقف‌شده، یا شروع مجدد تسک INACTIVE
os_task_stop(task_sensor);    // تعلیق غیرمخرب: TCB/پشته/وضعیت انتظار حفظ می‌شود
bool active_now = os_task_isActive(task_sensor);
uint8_t state   = os_task_get_state(task_sensor);     // TaskState به‌صورت uint8_t (0xFF اگر نامعلوم)
```

`os_task_stop()`/`os_task_start()` معادل CMSIS-RTOS2 `osThreadSuspend`/`osThreadResume` هستند: تسکی که در حالت انتظار روی یک event متوقف شده، هنگام start دوباره به انتظار روی همان event ادامه می‌دهد.

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
os_delay_ms(100);     // تأخیر blocking (از ISR یا داخل OS_SAFE کد SAFE_DELAY_MS می‌دهد)
os_delay_us(10);      // انتظار مشغول؛ yield نمی‌کند
os_yield();           // نقطه زمان‌بندی صریح

uint32_t tick = os_get_tick();
uint32_t ms   = os_get_ms();
uint32_t us   = os_get_us();   // مبتنی بر DWT با انباشتگر مقاوم به wrap
```

تیک پیش‌فرض ۱۰۰ میکروثانیه است و از `OS_KERNEL_TICK_PERIOD_US` (100..1000، باید ۱۰۰۰ را بدون باقیمانده تقسیم کند) قابل تنظیم است.

`os_get_us()` از توسعه مقاوم به wrap شمارنده DWT استفاده می‌کند؛ پس از تغییر `SystemCoreClock`، پنجره‌های بعدی خودبه‌خود همگام می‌شوند.

## ۵. بخش بحرانی

```cpp
volatile uint32_t counter = 0;

OS_SAFE {
    counter++;
}
```

`OS_SAFE` وقفه‌ها را برای مدت بلوک غیرفعال می‌کند و با RAII وضعیت قبلی PRIMASK را برمی‌گرداند (تودرتویی با شمارنده عمق امن است). بخش بحرانی باید کوتاه باشد. با وجود DWT، حد تشخیصی `OS_SAFETY_MAX_CRITICAL_US` در تجاوز کد `SAFE_TOO_LONG` گزارش می‌کند.

داخل `OS_SAFE` عملیات blocking انجام نشود — `os_delay_ms()` و انتظار event با timeout به‌جای بلاک شدن، خطای `SAFE_DELAY_MS`/`SAFE_EVENT_WAIT` می‌دهند.

## ۶. Event

Eventها نیازمند `OS_IPC_TOOLS_EN=1` هستند.

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

در ISR از نسخه امن استفاده کنید:

```cpp
data_ready.signal_from_isr();
```

Eventها شمارنده داخلی دارند. عملیات mask (`ev1 | ev2`، `signal_from_isr(mask)`) به mask سی‌ودوبیتی محدود است (IDهای 0..31)؛ IDهای بالاتر از ۳۱ باید به‌صورت تکی استفاده شوند. نابود کردن event همه تسک‌های منتظر را با نتیجه شکست بیدار می‌کند.

## ۷. Mutex و IPC Ceiling

```cpp
OS_MUTEX uart_mutex(20);   // سقف = اولویت ۲۰

OS_LOCK(uart_mutex) {      // گارد RAII؛ OS_LOCK_T(mtx, timeout) برای نسخه زمان‌دار
    send_message();
}
```

سقف mutex باید برابر بالاترین اولویت تسک‌هایی باشد که آن mutex را می‌گیرند. با `lock()` مالک فوراً به سقف ارتقا می‌یابد و در `unlock()` نهایی به اولویت پایه برمی‌گردد (فیلد `mutex_held_count` قفل‌های تودرتو و چندگانه را صحیح مدیریت می‌کند). unlock تحویل مستقیم به بالاترین تسک منتظر انجام می‌دهد.

قفل دستی هم در دسترس است:

```cpp
if (uart_mutex.lock(1000)) {
    send_message();
    uart_mutex.unlock();
}
```

`OS_MUTEX` بازگشتی است: قفل مجدد توسط مالک شمارنده تو در تو را افزایش می‌دهد. نابود کردن mutex اولویت مالک ارتقایافته را برمی‌گرداند و منتظران را بیدار می‌کند.

## ۸. Queue

```cpp
OS_QUEUE<uint16_t, 8> sensor_queue;

sensor_queue.put(42);            // بدون بلاک (timeout صفر)
sensor_queue.put(100, 100);      // put بلاک‌شونده با ددلاین مطلق ۱۰۰ms

uint16_t value;
if (sensor_queue.get(value, 1000)) {
    process(value);
}
```

صف‌ها ظرفیت compile-time دارند. معنای timeout بر اساس ددلاین مطلق است: `put(item, 100)` تحت رقابت در کل حداکثر حدود ۱۰۰ms منتظر می‌ماند، نه ۱۰۰ms برای هر تکرار حلقه. در ISR از عملیات غیربلاک‌شونده اختصاصی استفاده کنید:

```cpp
sensor_queue.put_from_isr(value);
sensor_queue.get_from_isr(value);
```

## ۹. Semaphore

```cpp
OS_SEMAPHORE buffers(3);          // اولیه ۳، بدون سقف
OS_SEMAPHORE slots(0, 8);         // اولیه ۰، سقف ۸

if (buffers.wait(1000)) {
    use_buffer();
    buffers.signal();
}
```

سمافورها برای pool منابع و هماهنگی بین ISR و task مناسب هستند؛ شمارنده آن‌ها محدود است و `signal_from_isr()` برای زمینه وقفه موجود است. `wait()` بلاک‌شونده هم از قاعده ددلاین مطلق پیروی می‌کند.

## ۱۰. پایش (OS_MONITORING_EN)

پایش نیازمند `OS_MONITORING_EN=1` است؛ فراخوانی‌ها را با `#if OS_MONITORING_EN` گارد کنید:

```cpp
#if OS_MONITORING_EN
uint32_t peak = os_get_stack_usage(task_sensor);        // بایت‌های مصرف‌شده
uint8_t  cpu  = os_get_task_cpu_usage(task_sensor);     // درصد CPU هر تسک
uint8_t  total = os_get_cpu_usage_total();               // درصد CPU کل سیستم
uint8_t  win  = os_get_cpu_usage();                      // درصد CPU بازه‌ای

uint8_t n = os_get_stack_report_count();
for (uint8_t i = 0; i < n; i++) {
    os_stack_report_entry_t rep;
    os_get_stack_report(i, &rep);   // name, id, size_bytes, peak_bytes
}

uint32_t wm  = os_get_stack_watermark(task_id);          // بر اساس شناسه عددی تسک
uint32_t pct = os_get_stack_watermark_percent(task_id);
#endif
```

پایش deadline (بخشی از `OS_MONITORING_EN`):

```cpp
#if OS_MONITORING_EN
os_task_set_deadline(task_sensor, 50);   // ددلاین سخت ۵۰ms؛ صفر = غیرفعال
uint32_t misses = os_get_deadline_miss_count(task_sensor);
#endif
```

## ۱۱. ثبت خطا

```cpp
os_log_error(OSError::SENSOR_TIMEOUT,
             (uint8_t)ErrorSeverity::WARNING);

uint32_t total      = os_get_error_count();
uint32_t unexpected = os_get_unexpected_error_count();
OSError  last       = os_get_last_error();
```

با فعال بودن `OS_MONITORING_EN`، خطاها در یک بافر حلقه‌ای RAM با ظرفیت `OS_MONITORING_LOG_SIZE` ذخیره می‌شوند؛ `os_get_error_log_entry(i)`، `os_get_error_log_count()` و `os_get_error_log_total()` آن را می‌خوانند. خطاهای تشخیص‌داده‌شده توسط هسته هم خودکار ثبت می‌شوند.

برای تزریق خطای عمدی (تست‌ها)، محدوده را طوری علامت‌گذاری کنید که خطاهای مورد انتظار غیرمنتظره شمرده نشوند:

```cpp
os_error_expect_begin();
    os_log_error(OSError::SENSOR_TIMEOUT, (uint8_t)ErrorSeverity::WARNING);
os_error_expect_end();
```

کدهای خطای تشخیص‌داده‌شده توسط هسته شامل `STACK_OVERFLOW`، `INVALID_EVENT_ID`، `TASK_AFTER_START`، `TASK_STUCK`، `SAFE_TOO_LONG`، `HARDFAULT`، `DEADLINE_MISS`، `TCB_CORRUPTED`، `SAFE_MUTEX_LOCK`، `RAM_TEST_FAIL` و `MPU_CONFIG_ERROR` هستند. `PRIORITY_CONFLICT` و `SENSOR_TIMEOUT` برای استفاده سطح برنامه رزرو شده‌اند.

## ۱۲. مکانیسم‌های ایمنی

ویژگی‌های ایمنی مستقلاً قابل پیکربندی‌اند:

- `OS_SAFETY_SOFT_WATCHDOG_EN`
- `OS_SAFETY_HW_WATCHDOG_EN`
- `OS_SAFETY_RAM_TEST_EN`
- `OS_SAFETY_CRC_EN`
- `OS_SAFETY_MPU_EN`

توابع پرس‌وجو/تغذیه همیشه تعریف شده‌اند و در حالت غیرفعال به stub بی‌اثر تبدیل می‌شوند:

```cpp
#if OS_SAFETY_RAM_TEST_EN
    os_ram_test_step();               // در هر فراخوانی یک کلمه، از idle
    if (os_ram_test_complete() && os_get_ram_test_error_count() > 0) { /* خطای SRAM */ }
#endif

#if OS_SAFETY_CRC_EN
    os_crc_check_step();              // os_crc_init() خودکار در os_init() اجرا می‌شود
    if (os_crc_check_complete() && os_get_crc_error_count() > 0) { /* فلش تغییر کرده */ }
#endif

#if OS_SAFETY_HW_WATCHDOG_EN
    os_hw_watchdog_feed();            // تغذیه بی‌قید و شرط
    os_hw_watchdog_check();           // تغذیه فقط در سلامت
    uint32_t r = os_get_hw_wdg_reset_count();  // خواندن/پاک کردن پرچم ریست IWDG
#endif

#if OS_SAFETY_MPU_EN
    os_mpu_add_region(tcb_ptr, base, size, attrs);  // ناحیه اضافی هر تسک
#endif
```

پروفایل‌های IEC 62304 و IEC 61508 بررسی‌های compile-time برای کامل‌بودن پیکربندی ZenOS هستند و به‌تنهایی گواهی محصول محسوب نمی‌شوند.

## ۱۳. کشف پلتفرم و قابلیت‌ها

```cpp
printf("family: %s\n", os_get_family_name());   // مثلاً "STM32F1"
printf("arch:   %s\n", os_get_arch_name());     // مثلاً "ARMv7-M (Cortex-M3)"

uint32_t caps = os_get_capabilities();
if (caps & OS_CAP_MPU_ACTIVE) { /* تسک‌ها تحت MPU اجرا می‌شوند */ }
if (caps & OS_CAP_CYCCNT)     { /* زمان‌بندی میکروثانیه DWT موجود است */ }
```

`os_get_capabilities()` یک bitmask برمی‌گرداند (`OS_CAP_MPU`، `OS_CAP_FPU`، `OS_CAP_MPU_ACTIVE`، `OS_CAP_CRC_HW`، `OS_CAP_IWDG_HW`، `OS_CAP_CYCCNT`، `OS_CAP_VTOR`، `OS_CAP_TICKLESS`) — راه استاندارد برای انشعاب بر اساس ویژگی‌های هدف به‌جای حدس زدن نام MCU. نسخه: `os_get_version()` / `os_get_version_string()`.

## ۱۴. مدل حافظه

منابع اصلی تسک‌ها به‌صورت ایستا مدیریت می‌شوند و مدل تسک‌های هسته از Heap عمومی استفاده نمی‌کند. اندازه پیش‌فرض پشته ۱۲۸ بایت با کف مؤثر ۲۵۶ بایت است و با `os_task_create_st()` قابل تغییر است.

قبل از انتخاب پشته‌های کوچک‌تر، مصرف پیک پشته را با `os_get_stack_usage()` / `os_get_stack_report()` روی همان پیکربندی کامپایلر اندازه بگیرید.

## ۱۵. الگوی pipeline

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

## ۱۶. مرجع پیکربندی

جزئیات همه گزینه‌ها در [CONFIG_GUIDE_FA.md](CONFIG_GUIDE_FA.md) و فایل مرجع [`ZenOS_Config.hpp`](../ZenOS/ZenOS_Config.hpp) قرار دارد.

---

**ZenOS RTOS v1.1.0 · ARM Cortex-M · C++11 · MIT License**

</div>
