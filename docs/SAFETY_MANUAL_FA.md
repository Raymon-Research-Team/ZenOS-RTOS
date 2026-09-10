# راهنمای ایمنی ZenOS RTOS

این سند مکانیسم‌های ایمنی، فرضیات یکپارچه‌سازی و محدودیت‌های پیاده‌سازی فعلی ZenOS را توضیح می‌دهد.

## ۱. جایگاه ایمنی

ZenOS مجموعه‌ای از مکانیسم‌های قابل پیکربندی برای توسعه سیستم‌های تعبیه‌شده مقاوم و حساس ارائه می‌کند. این مکانیسم‌ها به‌تنهایی به معنی گواهی ZenOS یا محصول نهایی بر اساس IEC 62304، IEC 61508، ISO 26262، DO-178C یا استاندارد دیگر نیستند.

تحلیل ریسک، نیازمندی‌ها، ردیابی، Verification، Validation و مدارک لازم برای محصول بر عهده یکپارچه‌ساز است.

## ۲. مدل زمان‌بند

- اولویت `0` برای idle رزرو است.
- با `OS_KERNEL_MAX_PRIORITIES=256`، اولویت‌های کاربردی **`1..255`** هستند.
- محدوده تنظیم تعداد اسلات‌های اولویت `2..256` است.
- مقدار پیش‌فرض ۳۲ اسلات و محدوده کاربردی پیش‌فرض `1..31` است.
- برای هر سطح اولویت صف آماده جداگانه وجود دارد.
- bitmap برای یافتن سطح اولویت فعال استفاده می‌شود.
- scheduler پیش از انتخاب task، eligibility را بررسی می‌کند.
- تسک دوره‌ای از `period_ticks` و `next_run_time` برای release gate استفاده می‌کند.

## ۳. منابع ایستا

اندازه پیش‌فرض پشته هر تسک ۵۱۲ بایت است. پشته سفارشی از طریق `os_task_create_st()` قابل تعیین است.

اندازه پشته باید روی MCU، compiler و optimization نهایی اندازه‌گیری شود.

## ۴. مکانیسم‌های ایمنی

### Stack protection

بررسی canary و محدوده SP برای تشخیص خطاهای پشته استفاده می‌شود. سیاست بازیابی با `OS_SAFETY_TASK_MAX_RECOVERY` و تنظیمات مربوط به واکنش به خطا کنترل می‌شود.

### TCB integrity

`OS_MONITOR_TCB_INTEGRITY` بررسی magic number در TCB را فعال می‌کند.

### Deadline

`OS_MONITOR_DEADLINE` پایش deadline را فعال می‌کند و واکنش با `OS_SAFETY_DEADLINE_ACTION` مشخص می‌شود:

- `0`: فقط ثبت خطا
- `1`: reset تسک
- `2`: غیرفعال‌کردن تسک

### Software watchdog

`OS_SAFETY_SOFT_WATCHDOG` تسک‌هایی را که در بازه `OS_SAFETY_SOFT_WDG_TIMEOUT_MS` واگذاری یا blocking مناسب ندارند تشخیص می‌دهد.

### Hardware watchdog

`OS_SAFETY_HW_WATCHDOG` مسیر feed/check برای IWDG STM32 را فراهم می‌کند. خود IWDG باید برای سخت‌افزار هدف در CubeMX تنظیم شود.

### MPU

`OS_SAFETY_MPU` حفاظت حافظه تسک را روی Cortex-Mهایی که MPU مناسب دارند فعال می‌کند. تعداد region و محدودیت‌های آن وابسته به سخت‌افزار است.

### RAM test

`OS_SAFETY_RAM_TEST` آزمون تدریجی incremental SRAM integrity routine را از طریق `os_ram_test_step()` فراهم می‌کند. اجرای آن باید با توجه به DMA و دسترسی همزمان به RAM طراحی شود.

### CRC

`OS_SAFETY_CRC_CHECK` بررسی تدریجی integrity فلش را از طریق peripheral CRC فراهم می‌کند. حافظه برنامه هنگام بررسی نباید تغییر کند.

### Error log

`OS_MONITOR_ERROR_LOG` یک بافر حلقه‌ای RAM فراهم می‌کند. مقدار پیش‌فرض `OS_MONITOR_ERROR_LOG_SIZE` برابر ۳۲ ورودی است. این log با reset از بین می‌رود مگر برنامه آن را ذخیره کند.

### Critical section

`OS_SAFETY_MAX_CRITICAL_US` حد تشخیصی مدت اجرای `OS_SAFE` را تعیین می‌کند. بخش‌های بحرانی طولانی latency وقفه را افزایش می‌دهند.

## ۵. مقادیر پیش‌فرض فعلی

| گزینه | پیش‌فرض |
|---|---:|
| `OS_MONITOR_DEADLINE` | 0 |
| `OS_MONITOR_TCB_INTEGRITY` | 0 |
| `OS_MONITOR_ERROR_LOG` | 1 |
| `OS_SAFETY_RAM_TEST` | 0 |
| `OS_SAFETY_MPU` | 0 |
| `OS_SAFETY_HW_WATCHDOG` | 0 |
| `OS_SAFETY_CRC_CHECK` | 0 |
| `OS_SAFETY_SOFT_WATCHDOG` | 0 |
| `OS_SAFETY_DEADLINE_ACTION` | 1 |
| `OS_SAFETY_TASK_MAX_RECOVERY` | 3 |
| `OS_SAFETY_SOFT_WDG_TIMEOUT_MS` | 3000 |
| `OS_SAFETY_MAX_CRITICAL_US` | 1000 |

مکانیسم‌های ایمنی مستقل از یکدیگر قابل فعال‌سازی هستند.

## ۶. پروفایل‌های IEC

ZenOS پروفایل‌های compile-time زیر را فراهم می‌کند:

- `OS_TARGET_MEDICAL=1..3` برای Class A/B/C
- `OS_TARGET_INDUSTRIAL=1..4` برای SIL 1..4

این پروفایل‌ها کامل‌بودن تنظیمات ZenOS را بررسی می‌کنند و به‌تنهایی گواهی محصول نیستند.

## ۷. فرضیات یکپارچه‌سازی

- همه تسک‌ها قبل از `os_start()` ساخته شوند.
- در ISR از نسخه‌های ISR-safe مربوط به IPC استفاده شود.
- داخل `OS_SAFE` عملیات blocking انجام نشود.
- سقف mutex برابر بالاترین اولویت تسک استفاده‌کننده باشد.
- مصرف واقعی stack اندازه‌گیری شود.
- IWDG و CRC در صورت استفاده در پروژه سخت‌افزار تنظیم شوند.
- قابلیت MPU قبل از فعال‌سازی روی MCU بررسی شود.
- برای faultهای تشخیص‌داده‌شده، رفتار recovery و safe-state در سطح محصول تعریف شود.

## ۸. محدودیت‌ها

مکانیسم‌های ایمنی ابزارهای تشخیص و واکنش هستند و تضمین نمی‌کنند که هر نوع خطای نرم‌افزاری یا سخت‌افزاری شناسایی شود.

- stack check جایگزین طراحی صحیح stack نیست.
- magic number همه انواع corruption را تشخیص نمی‌دهد.
- deadline monitoring تکمیل منطقی کار را ثابت نمی‌کند.
- RAM test همه انواع خطاهای RAM را پوشش نمی‌دهد.
- CRC یک integrity check است، نه احراز هویت رمزنگاری‌شده.
- MPU به قابلیت‌های سخت‌افزار و پیکربندی صحیح region وابسته است.
- error log در RAM است و با reset از بین می‌رود.
- watchdog reset وضعیت volatile را از بین می‌برد.

## ۹. چک‌لیست پیش از استفاده در محصول

1. commit و toolchain نهایی را ثابت کنید.
2. تنظیمات `ZenOS_Config.hpp` و target profile را ثابت کنید.
3. peak stack همه تسک‌ها را اندازه بگیرید.
4. timing و deadlineها را روی سخت‌افزار واقعی بررسی کنید.
5. مسیرهای fault و recovery را تست کنید.
6. watchdog، MPU، RAM test و CRC فعال را جداگانه تست کنید.
7. رفتار reset و safe-state محصول را مشخص کنید.
8. داده‌های تشخیصی موردنیاز را خارج از RAM-only error log ذخیره کنید.
9. فرایند کامل ایمنی موردنیاز حوزه محصول را اجرا کنید.

---

**ZenOS RTOS v1.0.1 · MIT License · Raymon Research Team**
