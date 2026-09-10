<div align="center">
<img src="guide-html/ZenOS_logo.svg" alt="ZenOS Logo" width="400" />
</div>

# راهنمای پیکربندی ZenOS RTOS

این سند رفتار و مقادیر پیکربندی فعلی ZenOS را توضیح می‌دهد. فایل مرجع اصلی، `ZenOS/ZenOS_Config.hpp` است.

## ۱. هسته

### `OS_KERNEL_TICK_PERIOD_US`

- پیش‌فرض: `100` میکروثانیه
- محدوده: `100..1000`
- مقدار باید `1000` را بدون باقیمانده تقسیم کند.

### `OS_KERNEL_STACK_SIZE`

- پیش‌فرض: `512` بایت
- حداقل: `64` بایت

این مقدار اندازه پیش‌فرض پشته هر تسک است. برای کاهش آن، مصرف واقعی پشته را روی همان MCU و تنظیمات کامپایل اندازه‌گیری کنید.

### `OS_KERNEL_MAX_PRIORITIES`

- پیش‌فرض: `32` اسلات
- محدوده: `2..256`
- اولویت `0`: رزرو برای idle
- در پیکربندی ۲۵۶ اسلات: **اولویت‌های کاربردی `1..255`**

```cpp
#define OS_KERNEL_MAX_PRIORITIES 32
```

هر مقدار اولویت یک سطح واقعی زمان‌بندی است و band یا نگاشت `priority >> 3` وجود ندارد.

| اسلات | اولویت‌های کاربردی | حافظه صف‌های اولویت | حافظه bitmap |
|---:|---:|---:|---:|
| 32 | 1..31 | 128 B | 4 B |
| 64 | 1..63 | 256 B | 8 B |
| 128 | 1..127 | 512 B | 16 B |
| 256 | 1..255 | 1024 B | 32 B |

این اعداد فقط حافظه ساختارهای مربوط به priority scheduler را نشان می‌دهند، نه کل RAM سیستم.

## ۲. ابزارها

| گزینه | پیش‌فرض | کاربرد |
|---|---:|---|
| `OS_TOOL_EVENT` | 1 | event |
| `OS_TOOL_MUTEX` | 1 | mutex و IPC ceiling |
| `OS_TOOL_QUEUE` | 1 | صف FIFO محدود |
| `OS_TOOL_SEMAPHORE` | 1 | semaphore شمارشی |
| `OS_TOOL_TICKLESS_IDLE` | 1 | idle مبتنی بر WFI |

## ۳. پایش

| گزینه | پیش‌فرض | کاربرد |
|---|---:|---|
| `OS_MONITOR_DEADLINE` | 0 | پایش deadline |
| `OS_MONITOR_TCB_INTEGRITY` | 0 | بررسی integrity TCB |
| `OS_MONITOR_ERROR_LOG` | 1 | ثبت خطای حلقه‌ای در RAM |
| `OS_MONITOR_ERROR_LOG_SIZE` | 32 | تعداد ورودی‌های log |

## ۴. مکانیسم‌های ایمنی

| گزینه | پیش‌فرض | کاربرد |
|---|---:|---|
| `OS_SAFETY_RAM_TEST` | 0 | آزمون تدریجی March C- |
| `OS_SAFETY_MPU` | 0 | حفاظت MPU هر تسک در سخت‌افزارهای پشتیبانی‌شده |
| `OS_SAFETY_HW_WATCHDOG` | 0 | اتصال به IWDG |
| `OS_SAFETY_CRC_CHECK` | 0 | بررسی CRC حافظه برنامه |
| `OS_SAFETY_SOFT_WATCHDOG` | 0 | تشخیص تسک گیرکرده |
| `OS_SAFETY_DEADLINE_ACTION` | 1 | `0=log`، `1=reset`، `2=disable` |
| `OS_SAFETY_TASK_MAX_RECOVERY` | 3 | حداکثر تلاش بازیابی |
| `OS_SAFETY_SOFT_WDG_TIMEOUT_MS` | 3000 | تایم‌اوت watchdog نرم‌افزاری |
| `OS_SAFETY_MAX_CRITICAL_US` | 1000 | حد تشخیصی مدت `OS_SAFE` |

مکانیسم‌های ایمنی همگی به‌صورت یکسان فعال نیستند؛ هر گزینه مقدار پیش‌فرض مستقل خود را دارد.

## ۵. پروفایل‌های هدف IEC

```text
-DOS_TARGET_MEDICAL=1   # Class A
-DOS_TARGET_MEDICAL=2   # Class B
-DOS_TARGET_MEDICAL=3   # Class C

-DOS_TARGET_INDUSTRIAL=1   # SIL 1
-DOS_TARGET_INDUSTRIAL=2   # SIL 2
-DOS_TARGET_INDUSTRIAL=3   # SIL 3
-DOS_TARGET_INDUSTRIAL=4   # SIL 4
```

فعال‌کردن پروفایل باعث می‌شود تنظیمات الزامی ZenOS در زمان کامپایل بررسی شوند. این بررسی‌ها گواهی محصول نیستند.

## ۶. پیکربندی کم‌حجم نمونه

```cpp
#define OS_KERNEL_TICK_PERIOD_US  1000UL
#define OS_KERNEL_STACK_SIZE      256

#define OS_TOOL_EVENT             1
#define OS_TOOL_MUTEX             1
#define OS_TOOL_QUEUE             0
#define OS_TOOL_SEMAPHORE         0
#define OS_TOOL_TICKLESS_IDLE     0

#define OS_MONITOR_DEADLINE       0
#define OS_MONITOR_TCB_INTEGRITY  0
#define OS_MONITOR_ERROR_LOG      0

#define OS_SAFETY_RAM_TEST        0
#define OS_SAFETY_MPU             0
#define OS_SAFETY_HW_WATCHDOG     0
#define OS_SAFETY_CRC_CHECK       0
#define OS_SAFETY_SOFT_WATCHDOG   0
```

این فقط یک نمونه برای سیستم محدود از نظر RAM/Flash است. در سیستم ایمنی‌محور، الزامات پروفایل هدف اولویت دارند.

## ۷. مقادیر مشتق‌شده

در `ZenOS.hpp` این مقادیر به‌صورت خودکار محاسبه می‌شوند:

- `OS_PRIORITY_BITMAP_WORDS`
- `OS_PRIORITY_EXTRA_COUNT`
- `OS_PRIORITY_EXTRA_WORDS`
- `OS_TICKS_PER_MS`
- `OS_IDLE_STACK_WORDS`
- `OS_FAULT_STACK_WORDS`

آن‌ها را مستقیماً تغییر ندهید.

## ۸. نکات عملی

- اگر به کل محدوده اولویت نیاز دارید، `OS_KERNEL_MAX_PRIORITIES=256` محدوده کاربردی `1..255` را فراهم می‌کند.
- برای مصرف کمتر RAM، تعداد اسلات‌های اولویت را متناسب با پروژه انتخاب کنید.
- ویژگی‌های IPC و پایش بدون استفاده را غیرفعال کنید.
- اندازه پشته را با گزارش runtime بررسی کنید.
- ویژگی‌های وابسته به سخت‌افزار مانند MPU، CRC و IWDG را فقط روی هدف مناسب فعال کنید.

---

**ZenOS RTOS v1.0.1 · MIT License · Raymon Research Team**
