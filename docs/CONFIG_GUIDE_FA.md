<div align="center">
<img src="guide-html/ZenOS_logo.svg" alt="ZenOS Logo" width="400" />
</div>

<div dir="rtl">

# راهنمای پیکربندی ZenOS RTOS

این سند رفتار و مقادیر پیکربندی فعلی ZenOS را توضیح می‌دهد. فایل مرجع اصلی، `ZenOS/ZenOS_Config.hpp` است.

## ۱. هسته

### `OS_KERNEL_TICK_PERIOD_US`

- پیش‌فرض: `100` میکروثانیه
- محدوده: `100..1000`
- مقدار باید `1000` را بدون باقیمانده تقسیم کند.

### `OS_KERNEL_DEFAULT_STACK_SIZE`

- پیش‌فرض: `128` بایت
- حداقل: `64` بایت (با `static_assert` بررسی می‌شود)

این مقدار اندازه پیش‌فرض پشته هر تسک است. الگوی ایجاد تسک یک کف مؤثر `256` بایتی اعمال می‌کند: درخواست‌های کوچک‌تر هم ۲۵۶ بایت فضا تخصیص می‌گیرند، چون راه‌اندازی پشته در هسته حداقل ۶۴ کلمه (۲۵۶ بایت) را پر می‌کند. برای کاهش مصرف، مصرف واقعی پشته را روی همان MCU و تنظیمات کامپایل با گزارش پشته runtime اندازه‌گیری کنید.

پشته سفارشی هر تسک از طریق `os_task_create_st(entry, prio, period, stack_bytes)` قابل تعیین است.

### `OS_KERNEL_MAX_PRIORITIES`

- پیش‌فرض: `16` اسلات
- محدوده: `2..256`
- اولویت `0`: رزرو برای idle
- محدوده کاربردی برنامه: `1..OS_KERNEL_MAX_PRIORITIES-1`
- در پیکربندی ۲۵۶ اسلات: **اولویت‌های کاربردی `1..255`**

قرارداد اولویت (مطابق CMSIS-RTOS2، FreeRTOS و ThreadX): **عدد بزرگ‌تر یعنی اولویت بالاتر**. `OS_KERNEL_MAX_PRIORITIES-1` بالاترین اولویت قابل استفاده است؛ اولویت `1` پایین‌ترین اولویت برنامه است؛ اولویت `0` مربوط به idle است و تا وقتی تسک کاربری آماده باشد اجرا نمی‌شود.

```cpp
#define OS_KERNEL_MAX_PRIORITIES 16
```

هر مقدار اولویت یک سطح واقعی زمان‌بندی است و band یا نگاشت `priority >> 3` وجود ندارد.

| اسلات | اولویت‌های کاربردی | حافظه صف‌های اولویت | حافظه bitmap |
|---:|---:|---:|---:|
| 32 | 1..31 | 128 B | 4 B |
| 64 | 1..63 | 256 B | 8 B |
| 128 | 1..127 | 512 B | 16 B |
| 256 | 1..255 | 1024 B | 32 B |

این اعداد فقط حافظه ساختارهای مربوط به priority scheduler را نشان می‌دهند، نه کل RAM سیستم.

### `OS_KERNEL_TICKLESS_IDLE_EN`

- پیش‌فرض: `1` (فعال)

idle بدون تیک: وقتی idle اجرا می‌شود و تسک‌ها بلاک‌اند، هسته SysTick را برای بیدار شدن بعدی برنامه‌ریزی می‌کند و در بیداری `tick_count` را به اندازه تیک‌های ردشده جلو می‌برد (صرفه‌جویی مصرف با `WFI`). برای زمان‌بندی ساده‌تر می‌توانید آن را غیرفعال کنید.

### پشته‌های idle و fault (مشتق‌شده، قابل بازنویسی)

| ماکرو | پیش‌فرض | توضیح |
|---|---:|---|
| `OS_IDLE_STACK_WORDS` | ۱۲۸ کلمه (۵۱۲ بایت) | پشته تسک idle. حلقه idle بررسی watchdog سخت‌افزاری، گام CRC و مسیر tickless را اجرا می‌کند و هر وقفه‌ای در حالت idle یک exception frame روی همین پشته می‌گذارد. با `-DOS_IDLE_STACK_WORDS=n` قابل تغییر است. |
| `OS_FAULT_STACK_WORDS` | ۶۴ کلمه (۲۵۶ بایت) | پشته fault handler برای ثبت زمینه خطا و بازیابی به idle. |

## ۲. ارتباط بین‌تسکی (IPC)

### `OS_IPC_TOOLS_EN`

| ویژگی | مقدار |
|---|---|
| پیش‌فرض | `0` |
| کاربرد | کلید اصلی واحد برای همه اصول ارتباط بین‌تسکی |

با `OS_IPC_TOOLS_EN=1` همه IPC با هم فعال می‌شوند: `OS_EVENT`، `OS_MUTEX` (با سقف اولویت فوری)، `OS_QUEUE`، `OS_SEMAPHORE` و گاردهای `OS_LOCK`/`OS_LOCK_T`. با `OS_IPC_TOOLS_EN=1` کلاس‌های IPC اصلاً کامپایل نمی‌شوند و برنامه نباید از آن‌ها استفاده کند. کلید جداگانه برای هرprimitive وجود ندارد.

> توجه برای پروفایل‌های ایمنی: اجرای IEC 61508 در SIL 2+ الزام `OS_IPC_TOOLS_EN=1` را اعمال می‌کند (قطع‌سازی متقابل برای منابع مشترک).

## ۳. پایش

| گزینه | پیش‌فرض | کاربرد |
|---|---:|---|
| `OS_MONITORING_EN` | 0 | کلید اصلی: پایش deadline + بررسی یکپارچگی TCB + ثبت خطا در RAM + اندازه‌گیری CPU و stack، همه با هم |
| `OS_MONITORING_DEADLINE_ACTION` | 1 | واکنش به از دست رفتن deadline (زیرگروه `OS_MONITORING_EN`): 0 = فقط ثبت، 1 = ریست تسک، 2 = غیرفعال کردن تسک؛ محدوده مجاز 0..2 |
| `OS_MONITORING_ERROR_LOG_SIZE` | 16 | تعداد ورودی‌های log خطا (زیرگروه `OS_MONITORING_EN`)؛ حداقل ۱ |

با `OS_MONITORING_EN=1` این‌ها کامپایل و اجرا می‌شوند (شرح کامل در بلوک کامنت `ZenOS_Config.hpp`):

1. **پایش ددلاین** — `os_task_set_deadline()` ددلاین سخت هر تسک را مسلح می‌کند؛ `os_tick()` از دست رفتن را تشخیص، شمارش و به‌صورت `DEADLINE_MISS` گزارش می‌کند؛ واکنش با `OS_MONITORING_DEADLINE_ACTION` تنظیم می‌شود؛ `os_get_deadline_miss_count()` شمارش هر تسک را می‌دهد.
2. **بررسی یکپارچگی TCB** — هر TCB عدد جادویی (`OS_TCB_MAGIC`) دارد که در ایجاد نوشته و پیش از اقدامات بازیابی watchdog بررسی می‌شود؛ TCB خراب با `TCB_CORRUPTED` گزارش و تسک به‌جای ریست، غیرفعال می‌شود.
3. **ثبت خطا** — `OS_MONITORING_ERROR_LOG_SIZE` ورودی با زمان، کد خطا، شناسه تسک و شدت. `_os_report_error()` هر خطای هسته را خودکار ثبت می‌کند. API پرس‌وجو: `os_log_error()`، `os_get_error_log_entry()`، `os_get_error_log_count()`، `os_get_error_log_total()`.
4. **بار CPU و اندازه‌گیری پشته** — `os_get_cpu_usage()`، `os_get_cpu_usage_total()`، `os_get_task_cpu_usage()`، ردیابی واترمارک peak-SP هر تسک، `os_get_stack_watermark()`، `os_get_stack_watermark_percent()` و جدول `os_get_stack_report()`.

با `OS_MONITORING_EN=0` همه موارد بالا حذف می‌شوند: TCB آن فیلدها را از دست می‌دهد، API ثبت خطا به stub بی‌اثر تبدیل می‌شود و توابع پایش کامپایل نمی‌شوند (فراخوانی‌های برنامه را با `#if OS_MONITORING_EN` گارد کنید).

هزینه RAM در حالت فعال (Cortex-M3، تقریبی): حدود ۸ بایت برای هر ورودی log (`OS_MONITORING_ERROR_LOG_SIZE` ورودی) + ۱۶ بایت برای هر تسک (فیلدهای deadline، magic و واترمارک).

## ۴. مکانیسم‌های ایمنی

| گزینه | پیش‌فرض | کاربرد |
|---|---:|---|
| `OS_SAFETY_RAM_TEST_EN` | 0 | آزمون پس‌زمینه March-C برای یکپارچگی SRAM (گام‌های تدریجی) |
| `OS_SAFETY_MPU_EN` | 0 | حفاظت MPU هر تسک در سخت‌افزارهای پشتیبانی‌شده |
| `OS_SAFETY_HW_WATCHDOG_EN` | 0 | اتصال به IWDG |
| `OS_SAFETY_CRC_EN` | 0 | بررسی تدریجی یکپارچگی فلش از طریق peripheral CRC |
| `OS_SAFETY_SOFT_WATCHDOG_EN` | 0 | تشخیص تسک گیرکرده و بازیابی |
| `OS_SAFETY_TASK_MAX_RECOVERY` | 3 | حداکثر تلاش بازیابی هر تسک قبل از غیرفعال‌سازی دائمی (۰ = حالت ایمن فوری) |
| `OS_SAFETY_SOFT_WDG_TIMEOUT_MS` | 3000 | تایم‌اوت watchdog نرم‌افزاری |
| `OS_SAFETY_MAX_CRITICAL_US` | 1000 | حداکثر مدت بخش `OS_SAFE` قبل از گزارش `SAFE_TOO_LONG` (۰ = غیرفعال) |

مکانیسم‌های ایمنی همگی به‌صورت یکسان فعال نیستند؛ هر گزینه مقدار پیش‌فرض مستقل خود را دارد.

گاردهای در دسترس‌بودن سخت‌افزار (در `ZenOS_Port.hpp`) هنگام درخواست ویژگی روی سخت‌افزار فاقد پشتیبانی، در زمان کامپایل خطا می‌دهند:

- `OS_SAFETY_MPU_EN=1` روی هدفی بدون MPU (`__MPU_PRESENT=0`، ARMv6-M) → `#error`
- `OS_SAFETY_CRC_EN=1` بدون peripheral CRC (تعریف‌نشده بودن `CRC_BASE`) → `#error`
- `OS_SAFETY_HW_WATCHDOG_EN=1` بدون IWDG (تعریف‌نشده بودن `IWDG_BASE`) → `#error`

## ۵. پروفایل‌های هدف IEC

```text
-DOS_TARGET_MEDICAL_CLASS=1   # Class A
-DOS_TARGET_MEDICAL_CLASS=2   # Class B
-DOS_TARGET_MEDICAL_CLASS=3   # Class C

-DOS_TARGET_INDUSTRIAL_SIL=1   # SIL 1
-DOS_TARGET_INDUSTRIAL_SIL=2   # SIL 2
-DOS_TARGET_INDUSTRIAL_SIL=3   # SIL 3
-DOS_TARGET_INDUSTRIAL_SIL=4   # SIL 4
```

| پروفایل | الزامات اعمال‌شده در زمان کامپایل |
|---|---|
| کلاس B/C پزشکی (`OS_TARGET_MEDICAL_CLASS >= 2`) | `OS_MONITORING_EN=0`، `OS_SAFETY_SOFT_WATCHDOG_EN=1`، `OS_MONITORING_DEADLINE_ACTION >= 1` |
| کلاس C پزشکی (`OS_TARGET_MEDICAL_CLASS >= 3`) | علاوه بر آن `OS_SAFETY_HW_WATCHDOG_EN=1`، `OS_SAFETY_MPU_EN=1`، `OS_SAFETY_CRC_EN=1`، `OS_SAFETY_RAM_TEST_EN=1` |
| SIL 1+ صنعتی (`OS_TARGET_INDUSTRIAL_SIL >= 1`) | `OS_SAFETY_SOFT_WATCHDOG_EN=1`، `OS_MONITORING_EN=0` |
| SIL 2+ صنعتی (`OS_TARGET_INDUSTRIAL_SIL >= 2`) | علاوه بر آن `OS_IPC_TOOLS_EN=1`، `OS_SAFETY_HW_WATCHDOG_EN=1`، `OS_MONITORING_DEADLINE_ACTION >= 1` |
| SIL 3+ صنعتی (`OS_TARGET_INDUSTRIAL_SIL >= 3`) | علاوه بر آن `OS_SAFETY_MPU_EN=1`، `OS_SAFETY_RAM_TEST_EN=1`، `OS_SAFETY_CRC_EN=1` |

اگر هر دو پروفایل تعریف شوند، الزام سخت‌تر اعمال می‌شود. فعال‌کردن پروفایل باعث می‌شود تنظیمات الزامی ZenOS در زمان کامپایل بررسی شوند. این بررسی‌ها کامل‌بودن پیکربندی را کنترل می‌کنند و گواهی محصول نیستند.

## ۶. پیکربندی کم‌حجم نمونه

```cpp
#define OS_KERNEL_TICK_PERIOD_US  1000UL
#define OS_KERNEL_DEFAULT_STACK_SIZE  256

#define OS_IPC_TOOLS_EN              1   // 0 یعنی حذف همه کلاس‌های IPC

#define OS_MONITORING_EN                1
#define OS_MONITORING_DEADLINE_ACTION         1
#define OS_MONITORING_ERROR_LOG_SIZE       16

#define OS_SAFETY_RAM_TEST_EN        0
#define OS_SAFETY_MPU_EN             0
#define OS_SAFETY_HW_WATCHDOG_EN     0
#define OS_SAFETY_CRC_EN             0
#define OS_SAFETY_SOFT_WATCHDOG_EN   0
```

این فقط یک نمونه برای سیستم محدود از نظر RAM/Flash است. در سیستم ایمنی‌محور، الزامات پروفایل هدف اولویت دارند.

## ۷. بازنویسی پیکربندی

هر گزینه با `#ifndef` محافظت شده است، بنابراین تعاریف سطح پروژه را می‌توان قبل از پردازش هدر ارائه کرد. گزینه‌های `-D` کامپایلر معمولاً تمیزترین راه برای جدا نگه‌داشتن پیکربندی خاص پروژه از سورس توزیع‌شده است.

اعتبارسنجی زمان کامپایل شامل:

- `OS_KERNEL_TICK_PERIOD_US` (محدوده 100..1000 و قاعده تقسیم بدون باقیمانده)
- `OS_KERNEL_MAX_PRIORITIES` (2..256)
- `OS_MONITORING_DEADLINE_ACTION` (0..2)
- `OS_MONITORING_ERROR_LOG_SIZE` (>= 1)
- محدوده‌های `OS_TARGET_MEDICAL_CLASS` / `OS_TARGET_INDUSTRIAL_SIL`
- همه الزامات پروفایل‌های هدف فهرست‌شده در بخش ۵

## ۸. مقادیر مشتق‌شده

این مقادیر به‌صورت خودکار محاسبه می‌شوند و نباید دستی تغییر کنند:

| ماکرو | توضیح | محل تعریف |
|---|---|---|
| `OS_PRIORITY_BITMAP_WORDS` | کلمات bitmap آماده برای همه اولویت‌های پیکربندی‌شده | `ZenOS.hpp` |
| `OS_PRIORITY_EXTRA_COUNT` | اسلات‌های صف بالای اولویت ۳۱ | `ZenOS.hpp` |
| `OS_PRIORITY_EXTRA_WORDS` | کلمات bitmap اولویت‌های توسعه‌ای | `ZenOS.hpp` |
| `OS_TICKS_PER_MS` | تبدیل تیک مشتق از `OS_KERNEL_TICK_PERIOD_US` | `ZenOS.hpp` |
| `OS_IDLE_STACK_SIZE` / `OS_FAULT_STACK_SIZE` | اندازه بایتی پشته idle/fault | `ZenOS.hpp` |
| `OS_WAIT_FOREVER` | ثابت تایم‌اوت بی‌نهایت | `ZenOS.hpp` |
| `OS_RAM_TEST_START` / `OS_RAM_TEST_END` | محدوده آزمون RAM (شروع = `__bss_end__` لینکر؛ پایان = نیمه پایین RAM، قابل بازنویسی با `-DOS_RAM_TEST_END=<addr>`) | `ZenOS_Port.hpp` |
| `OS_HAS_MPU_HW` / `OS_HAS_FPU_HW` / `OS_HAS_CRC_HW` / `OS_HAS_IWDG_HW` / `OS_HAS_CYCLE_COUNTER` / `OS_HAS_VTOR` | تشخیص قابلیت سخت‌افزار (هدر CMSIS + `__ARM_ARCH_*`) | `ZenOS_Port.hpp` |

## ۹. نکات عملی

- تعداد اسلات‌های اولویت را متناسب با پروژه انتخاب کنید. `256` اسلات محدوده کامل `1..255` را می‌دهد؛ پیش‌فرض ۱۶ اسلات است تا پیکربندی پایه کوچک بماند.
- برای کاهش مصرف RAM، ویژگی‌های IPC و پایش بدون استفاده را غیرفعال کنید (`OS_IPC_TOOLS_EN` و `OS_MONITORING_EN`).
- اندازه پشته را با گزارش runtime بررسی کنید و قبل از کاهش `OS_KERNEL_DEFAULT_STACK_SIZE` روی همان سطح بهینه‌سازی اندازه بگیرید.
- در پروژه‌های ایمنی‌محور، مکانیسم‌های لازم را صریحاً فعال و پروفایل هدف متناظر را تعیین کنید.
- ویژگی‌های وابسته به سخت‌افزار مانند MPU، CRC و IWDG را فقط روی هدف مناسب فعال کنید — گاردهای سطح پورت، ترکیب‌های ناممکن را رد می‌کنند.

---

**ZenOS RTOS v1.1.0 · MIT License · Raymon Research Team**

</div>
