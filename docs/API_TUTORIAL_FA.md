<div align="center">
<img src="./ZenOS_logo.svg" alt="ZenOS Logo" width="400" />
</div>

<div dir="rtl">

# آموزش جامع API سیستم‌عامل بلادرنگ ZenOS

 **پلتفرم:** ARM Cortex-M (STM32) | **زبان:** C++11 / C

> **📝 توجه:** تمام مثال‌ها و نمونه کدهای این آموزش برای **STM32F103C8T6** (Blue Pill، Cortex-M3، 72MHz، 64KB فلش، 20KB RAM) نوشته شده‌اند. کد قابل انتقال به خانواده‌های دیگر STM32 است — فقط نام پین‌ها و هندلرهای مد را مطابق برد خود تنظیم کنید.

</div>

---

<div dir="rtl">

## فهرست مطالب

۰. [شروع از صفر (راهنمای مبتدی)](#۰-شروع-از-صفر-راهنمای-مبتدی)
1. [شروع سریع](#۱-شروع-سریع)
2. [مدیریت تسک‌ها](#۲-مدیریت-تسک‌ها)
3. [زمان‌بندی و تأخیرها](#۳-زمان‌بندی-و-تأخیرها)
4. [بخش‌های حیاتی (OS_SAFE)](#۴-بخش‌های-حیاتی-os_safe)
5. [eventها (OS_EVENT)](#۵-eventها-os_event)
6. [قفل‌ها (OS_MUTEX)](#۶-قفل‌ها-os_mutex)
7. [صفوف (OS_QUEUE)](#۷-صفوف-os_queue)
8. [سیگنال‌ها (OS_SEMAPHORE)](#۸-سیگنال‌ها-os_semaphore)
9. [تسک‌های دوره‌ای — روش ZenOS (بدون تایمر نرم‌افزاری)](#۹-تسک‌های-دوره‌ای--روش-zenos-بدون-تایمر-نرم‌افزاری)
10. [مدیریت خطا و پایش](#۱۰-مدیریت-خطا-و-پایش)
11. [ویژگی‌های ایمنی](#۱۱-ویژگی‌های-ایمنی)
12. [نکات و ترفندهای پیشرفته](#۱۲-نکات-و-ترفندهای-پیشرفته)
13. [کتابخانه الگوهای رایج](#۱۳-کتابخانه-الگوهای-رایج)

</div>

---

<div dir="rtl">

## ۰. شروع از صفر (راهنمای مبتدی)

> **📝 توجه:** این بخش برای مبتدیانی است که قبلاً از STM32 یا ZenOS استفاده نکرده‌اند. اگر با CubeMX و توسعه ARM آشنا هستید، به [بخش ۱](#۱-شروع-سریع) بروید.

### ۰.۱ پیش‌نیازها — چه چیزی لازم دارید بدانید

پیش از شروع، باید دانش پایه‌ای از موارد زیر داشته باشید:

| موضوع | سطح | منابع |
|--------|------|--------|
| **برنامه‌نویسی C/C++** | پایه | متغیرها، توابع، حلقه‌ها، اشاره‌گرها |
| **RTOS چیست؟** | پایه | تسک‌ها، زمان‌بندی، اولویت‌ها |
| **ARM Cortex-M** | اختیاری | مفید اما الزامی نیست — ZenOS سخت‌افزار را انتزاع می‌کند |

> **💡 نگران نباشید اگر برنامه‌نویسی تعبیه‌شده بلد نیستید.** ZenOS بخش‌های سخت (زمان‌بندی، وقفه‌ها، محافظت حافظه) را مدیریت می‌کند. شما فقط تسک‌ها را می‌نویسید.

### ۰.۲ سخت‌افزار مورد نیاز

| قطعه | مشخصات | کجا بخریم |
|--------|---------|------------|
| **برد STM32F103C8T6** | "Blue Pill" — ~۲ دلار | علی‌اکسپرس، آمازون |
| **دیباگر ST-Link V2** | ~۳ دلار | علی‌اکسپرس، آمازون |
| **کابل USB** | Micro-USB (برای تغذیه + UART) | هر فروشگاه الکترونیکی |
| **سیم‌های جامپر** | ماده به نر، ۱۰+ عدد | هر فروشگاه الکترونیکی |
| **LED + مقاومت** | ۲۲۰Ω–۱kΩ (برای تست) | هر فروشگاه الکترونیکی |

**اختیاری اما پیشنهادی:**
- آداپتور USB به UART (CH340G یا CP2102) — برای خروجی سریال
- برِدبورد — برای نمونه‌سازی

**هزینه کل: ~۱۰ دلار**

</div>

```
┌─────────────────────────────────────────────────┐
│              تنظیم سخت‌افزار                      │
│                                                  │
│   [ST-Link V2]  ←→  [Blue Pill STM32F103C8]    │
│       SWD              Micro-USB                 │
│                          ↓                       │
│                    [پورت USB کامپیوتر]           │
│                                                  │
│   اختیاری:                                       │
│   [CH340G USB-UART] ←→ [PA9/PA10 روی Blue Pill] │
└─────────────────────────────────────────────────┘
```

<div dir="rtl">

### ۰.۳ نصب نرم‌افزار

#### مرحله ۱: نصب STM32CubeIDE

1. به [https://www.st.com/en/development-tools/stm32cubeide.html](https://www.st.com/en/development-tools/stm32cubeide.html) بروید
2. دانلود و نصب کنید (رایگان، حدود ۲GB)
3. اگر خواسته شد، یک حساب کاربری رایگان ST بسازید

> **💡 جایگزین:** می‌توانید از **PlatformIO** (افزونه VS Code) هم استفاده کنید اگر ترجیح می‌دهید.

#### مرحله ۲: نصب درایورهای ST-Link

1. ST-Link Utility را از وب‌سایت ST دانلود کنید
2. یا از طریق IDE نصب کنید (STM32CubeIDE درایورها را شامل می‌شود)

#### مرحله ۳: بررسی نصب

1. Blue Pill را از طریق USB وصل کنید
2. Device Manager (ویندوز) را باز کنید — باید `STMicroelectronics STLink` یا `USB Serial Device` را ببینید
3. اگر شناسایی نشد، درایورها را دستی نصب کنید

### ۰.۴ ساخت اولین پروژه CubeMX

#### مرحله ۱: STM32CubeIDE را باز کنید → File → New → STM32 Project

#### مرحله ۲: انتخاب MCU
- در MCU Selector، تایپ کنید `STM32F103C8`
- `STM32F103C8T6` (پکیج LQFP48) را انتخاب کنید
- روی "Start Project" کلیک کنید

#### مرحله ۳: پیکربندی ساعت
- به تب **Clock Configuration** بروید
- **HCLK** را روی `72 MHz` تنظیم کنید
- اگر CubeMX پیشنهاد PLL داد، "OK" بزنید

#### مرحله ۴: پیکربندی GPIO (برای LED)
- به تب **Pinout & Configuration** بروید
- روی `PC13` (LED داخلی Blue Pill) کلیک کنید
- آن را `GPIO_Output` تنظیم کنید

#### مرحله ۵: تنظیم زمان‌بند HAL روی TIM1
- به **System Core** → **SYS** بروید
- **Timebase Source** را روی `TIM1` تنظیم کنید
- این کار زمان‌بند HAL را از SysTick دور می‌کند تا ZenOS بتواند از SysTick برای تیک هسته استفاده کند

#### مرحله ۶: تولید کد
- روی تب **Project Manager** کلیک کنید
- نام پروژه را تنظیم کنید (مثلاً `ZenOS_Blink`)
- Toolchain: `STM32CubeIDE`
- **Generate Code** را کلیک کنید

### ۰.۵ اضافه کردن ZenOS به پروژه

#### مرحله ۱: دانلود ZenOS

</div>

```bash
git clone https://github.com/Raymon-Research-Team/ZenOS-RTOS.git
```

<div dir="rtl">

#### مرحله ۲: کپی فایل‌ها
این فایل‌ها را از `ZenOS/` به پروژه خود کپی کنید:

| از | به |
|----|----|
| `ZenOS.hpp` | `Inc/` |
| `ZenOS_c.h` | `Inc/` |
| `ZenOS_Config.hpp` | `Inc/` |
| `ZenOS_Internal.hpp` | `Inc/` |
| `ZenOS_Port.hpp` | `Inc/` |
| `ZenOS.cpp` | `Src/` |
| `ZenOS_Scheduler.cpp` | `Src/` |
| `ZenOS_IPC.cpp` | `Src/` |
| `ZenOS_Safety.cpp` | `Src/` |
| `ZenOS_Monitor.cpp` | `Src/` |

#### مرحله ۳: پیکربندی کامپایلر
- Project → Properties → C/C++ Build → Settings
- **MCU C++ Compiler** → Miscellaneous → اضافه کنید `-std=c++11`
- **MCU C++ Compiler** → Optimization → انتخاب کنید `-O2`

#### مرحله ۴: اضافه کردن os_tick() به SysTick_Handler

فایل `Src/stm32f1xx_it.c` را باز کنید و اضافه کنید:

</div>

```c
#include "ZenOS.hpp"  // در بالای فایل اضافه کنید

void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */
  os_tick();
  /* USER CODE END SysTick_IRQn 0 */
}
```

<div dir="rtl">

### ۰.۶ نوشتن اولین برنامه ZenOS

فایل `Src/main.cpp` را باز کنید و محتوا را جایگزین کنید:

</div>

```cpp
#include "ZenOS.hpp"

/* ─── تسک: چشمک زدن LED ─── */
void task_blink(void) {
    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);  // تغییر وضعیت LED
        os_delay_ms(500);                          // انتظار ۵۰۰ میلی‌ثانیه
    }
}

/* ─── تابع اصلی ─── */
int main(void) {
    HAL_Init();
    SystemClock_Config();  // توسط CubeMX تولید شده
    MX_GPIO_Init();        // توسط CubeMX تولید شده
    MX_TIM1_Init();        // توسط CubeMX تولید شده

    os_init();             // راه‌اندازی ZenOS
    os_task_create(task_blink, 5);  // ایجاد تسک با اولویت ۵
    os_start();            // شروع زمان‌بند (هرگز برنمی‌گردد!)

    while (1) {}  // هرگز اجرا نمی‌شود
}
```

<div dir="rtl">

### ۰.۷ کامپایل و فلش

1. روی **Build** (آیکون چکش) کلیک کنید — باید با ۰ خطا کامپایل شود
2. روی **Run** (آیکون پخش) کلیک کنید — به Blue Pill فلش شود
3. LED باید هر ۵۰۰ میلی‌ثانیه چشمک بزند! 🎉

### ۰.۸ اضافه کردن خروجی سریال (اختیاری)

برای دیدن پیام‌های ZenOS، UART را در CubeMX پیکربندی کنید:

1. **USART1** را فعال کنید (PA9=TX، PA10=RX)
2. نرخ بیت: `115200`
3. این تابع کمکی را در `main.cpp` اضافه کنید:

</div>

```cpp
#include <cstdio>

extern "C" int _write(int fd, char* ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}
```

<div dir="rtl">

4. حالا می‌توانید از `printf()` برای دیباگ استفاده کنید!

### ۰.۹ چه چیزی یاد گرفتید

| مفهوم | چه اتفاقی افتاد |
|--------|------------------|
| **تسک** | `task_blink` — تابعی که برای همیشه اجرا می‌شود |
| **اولویت** | `۵` — عدد بزرگتر = اولویت بالاتر |
| **os_delay_ms()** | تسک را به خواب می‌برد، به دیگران فرصت اجرا می‌دهد |
| **os_init()** | هسته ZenOS را راه‌اندازی می‌کند |
| **os_start()** | کنترل را به زمان‌بند می‌دهد |

### ۰.۱۰ اشتباهات رایج مبتدیان

| اشتباه | علامت | راه‌حل |
|---------|--------|--------|
| تسک `while(1)` ندارد | سیستم کرش می‌کند | همیشه از `while(1) {}` در تسک‌ها استفاده کنید |
| تسک واگذاری نمی‌کند | ریست تایمر نرم‌افزاری | `os_delay_ms()` یا `os_yield()` اضافه کنید |
| `os_tick()` فراموش شده | شمارنده تیک روی ۰ گیر کرده | `os_tick()` را به `SysTick_Handler` اضافه کنید |
| C++ فعال نشده | خطاهای کامپایل | `-std=c++11` را به پرچم‌های کامپایلر اضافه کنید |
| تسک بعد از `os_start()` ایجاد شده | مقدار -۱ برمی‌گرداند | تمام تسک‌ها را قبل از `os_start()` ایجاد کنید |

</div>

---

<div dir="rtl">

## ۰. راه‌اندازی CubeMX (پیش از اولین build)

> **📝 هدف MCU:** این دستورالعمل‌ها از **STM32F103C8T6** به‌عنوان مرجع استفاده می‌کنند. مقادیر ساعت و تنظیمات مد را برای MCU مخصوص خود تنظیم کنید.

پیش از استفاده از ZenOS، باید STM32CubeMX را به‌درستی پیکربندی کنید. این یک **راه‌اندازی یک‌بار** برای هر پروژه است.

### ۰.۱ انتقال زمان‌بند HAL از SysTick به TIM1

ZenOS از `SysTick` برای تیک هسته خود (`os_tick()`) استفاده می‌کند. HAL هم به یک زمان‌بند برای تایم‌اوت‌های داخلی خود (`HAL_Delay`، `HAL_GetTick`) نیاز دارد. اگر هر دو از SysTick استفاده کنند، تداخل ایجاد می‌شود.

**راه‌حل:** زمان‌بند HAL را به یک تایمر سخت‌افزاری (TIM1) منتقل کنید.

#### مرحله ۱: CubeMX → System Core → SYS → Timebase Source = TIM1

| تنظیم | مقدار |
|--------|--------|
| Timebase Source | `TIM1` |

CubeMX به‌طور خودکار TIM1 را با Prescaler و Period مناسب برای زمان‌بند 1ms پیکربندی می‌کند.

> **💡 نکته:** زمان‌بند پیش‌فرض HAL روی 1ms تنظیم شده. تیک هسته ZenOS به‌طور مستقل از طریق SysTick با 100μs (قابل پیکربندی) اجرا می‌شود.

#### مرحله ۲: تولید کد

CubeMX فایل `stm32f1xx_hal_timebase_tim.c` را با پیکربندی TIM1 تولید می‌کند.

#### مرحله ۳: اضافه کردن `os_tick()` به SysTick_Handler

فایل `Src/stm32f1xx_it.c` را باز کنید و `os_tick()` را داخل `SysTick_Handler` اضافه کنید:

</div>

```c
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */
  os_tick();
  /* USER CODE END SysTick_IRQn 0 */

  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}
```

<div dir="rtl">

> **⚠️ مهم:** `os_tick()` باید از `SysTick_Handler` فراخوانی شود — این تنها راه دریافت وقفه تیک توسط ZenOS است.

#### مرحله ۴: اضافه کردن هدر

در فایل `stm32f1xx_it.c`، include را در بالای فایل اضافه کنید:

</div>

```c
#include "ZenOS.hpp"
```

<div dir="rtl">

### ۰.۲ کپی فایل‌های ZenOS به پروژه خود

فایل‌های زیر را از مخزن ZenOS در پروژه CubeMX خود کپی کنید:

| مبدأ (ZenOS/) | مقصد (پروژه شما/) |
|---------------|-------------------|
| `ZenOS/ZenOS.hpp` | `Inc/` |
| `ZenOS/ZenOS_c.h` | `Inc/` |
| `ZenOS/ZenOS_Config.hpp` | `Inc/` |
| `ZenOS/ZenOS_Internal.hpp` | `Inc/` |
| `ZenOS/ZenOS_Port.hpp` | `Inc/` |
| `ZenOS/ZenOS.cpp` | `Src/` |
| `ZenOS/ZenOS_Scheduler.cpp` | `Src/` |
| `ZenOS/ZenOS_IPC.cpp` | `Src/` |
| `ZenOS/ZenOS_Safety.cpp` | `Src/` |
| `ZenOS/ZenOS_Monitor.cpp` | `Src/` |

### ۰.۳ تنظیمات کامپایلر

ZenOS از **ARM GCC** inline assembly، `__attribute__((naked))` و ویژگی‌های C++11 استفاده می‌کند. ابزارک **`arm-none-eabi-gcc` الزامی** است.

**کامپایلرهای پشتیبانی‌شده:**

| کامپایلر | پشتیبانی | توضیحات |
|----------|----------|---------|
| **ARM GCC (arm-none-eabi-gcc)** | ✅ **الزامی** | نسخه ≥ 6.x پیشنهاد می‌شود |
| **STM32CubeIDE** | ✅ | از ARM GCC استفاده می‌کند |
| **PlatformIO (GCC)** | ✅ | از ARM GCC استفاده می‌کند |
| **ARM Keil (ARMCC v5)** | ❌ | نحوه `__attribute__((naked))` متفاوت است |
| **ARM Keil (ARMClang v6)** | ⚠️ | ممکن است کار کند — با `__attribute__((naked))` تست کنید |
| **IAR EWARM** | ❌ | نحوه inline assembly متفاوت است |

**پرچم‌های الزامی کامپایلر:**

</div>

```
-std=c++11        # C++11 الزامی (template، static_assert، nullptr، RAII)
-mcpu=cortex-m3    # برای MCU خود تنظیم کنید: m3, m4, m7, m0plus
-mthumb            # دستورات Thumb
-O2                # سطح بهینه‌سازی پیشنهادی
```

<div dir="rtl">

**تنظیم STM32CubeIDE:**
- Project → Properties → C/C++ Build → Settings
- **MCU C++ Compiler** → Miscellaneous → `-std=c++11 -mcpu=cortex-m3 -mthumb`
- **MCU C++ Compiler** → Optimization → `-O2`

**تنظیم PlatformIO (`platformio.ini`):**

</div>

```ini
[env:bluepill]
platform = ststm32
board = bluepill_f103c8
framework = stm32hal
compiler.cppflags = -std=c++11
```

<div dir="rtl">

> **⚠️ مهم:**
> 1. فایل‌های `.cpp` **باید** به‌عنوان C++ کامپایل شوند (نه C)
> 2. `arm-none-eabi-gcc` ≥ 6.x برای پشتیبانی کامل C++11 الزامی است
> 3. پرچم `-mcpu` باید با هسته MCU شما مطابقت داشته باشد (m3 برای F1، m4 برای F4، m7 برای F7/H7)

### ۰.۴ پیکربندی مدها در CubeMX

بسته به اینکه کدام ویژگی‌های ایمنی را فعال می‌کنید، این مدها را در CubeMX پیکربندی کنید:

| ویژگی | تنظیم CubeMX |
|--------|---------------|
| تایمر سخت‌افزاری (`OS_SAFETY_HW_WATCHDOG`) | فعال‌سازی **IWDG** با تایم‌اوت ~6.5 ثانیه |
| بررسی CRC (`OS_SAFETY_CRC_CHECK`) | فعال‌سازی مده **CRC** |
| بقیه | تغییری در CubeMX لازم نیست (توسط ZenOS مدیریت می‌شود) |

> **💡 نکته:** PendSV، مدیرهای خطا (HardFault، MemManage، BusFault، UsageFault) و شمارنده چرخه DWT **به‌طور خودکار** توسط `os_init()` پیکربندی می‌شوند — نیازی به راه‌اندازی دستی نیست.

### ۰.۵ بررسی build

پس از راه‌اندازی، بررسی کنید:
1. `HAL_GetTick()` کار می‌کند (زمان‌بند HAL روی TIM1)
2. `os_get_ms()` کار می‌کند (تیک ZenOS روی SysTick)
3. تداخلی با SysTick وجود ندارد

### ۰.۶ چک‌لیست فایل‌ها

| فایل | چه چیزی بررسی شود |
|------|---------------------|
| `stm32f1xx_hal_timebase_tim.c` | توسط CubeMX تولید شده (TIM1 به‌عنوان زمان‌بند HAL) |
| `Src/stm32f1xx_it.c` | `os_tick()` در `SysTick_Handler` |
| `Inc/main.h` | `#include "ZenOS.hpp"` اگر از ماکروهای OS استفاده می‌کنید |
| `ZenOS_Config.hpp` | بررسی `OS_KERNEL_TICK_PERIOD_US` (پیش‌فرض 100μs) |
| تنظیمات کامپایلر | C++11 برای تمام فایل‌های `.cpp` فعال باشد |
| `ZenOS/ZenOS_Port.hpp` | خانواده MCU را خودکار تشخیص می‌دهد — نیازی به تغییر نیست |

</div>

---

<div dir="rtl">

## ۱. شروع سریع

### ۱.۱ اضافه کردن و راه‌اندازی

</div>

```cpp
#include "ZenOS.hpp"

int main() {
    HAL_Init();
    SystemClock_Config();
    // ... راه‌اندازی مدها (UART، GPIO و ...) ...

    os_init();  // راه‌اندازی اجزای داخلی سیستم‌عامل

    // ایجاد تسک‌ها قبل از os_start()
    os_task_create(task_main, 5);            // اولویت 5، پشته پیش‌فرض
    os_task_create(task_sensor, 3, 100);     // اولویت 3، دوره‌ای 100 میلی‌ثانیه
    os_task_create_st(task_ui, 8, 0, 1024); // اولویت 8، پشته سفارشی 1KB

    os_start();  // هرگز برنمی‌گردد — زمان‌بند همه‌چیز را کنترل می‌کند
}
```

<div dir="rtl">

### ۱.۲ حداقل تسک مورد نیاز

</div>

```cpp
void task_blink(void) {
    while (1) {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        os_delay_ms(500);  // تأخیر 500 میلی‌ثانیه و واگذاری CPU
    }
}
```

<div dir="rtl">

> **💡 نکته:** هر تسکی باید شامل `while(1)` باشد و باید به‌صورت دوره‌ای واگذاری کند (از طریق `os_delay_ms`، `os_yield` یا انتظار روی IPC). تسکی که هرگز واگذاری نکند توسط تایمر نرم‌افزاری ایمنی شناسایی و ریست می‌شود.

### ۱.۳ ترتیب راه‌اندازی

</div>

```
os_init()  →  ایجاد تسک‌ها  →  os_start()  →  زمان‌بند تا ابد اجرا می‌کند
    ↑                                                        ↓
    └──────── os_start() هرگز برنمی‌گردد ─────────────────────┘
```

<div dir="rtl">

**قانون حیاتی:** تمام تسک‌ها باید **قبل از** `os_start()` ایجاد شوند. ایجاد تسک بعد از `os_start()` مقدار -1 برمی‌گرداند و `TASK_AFTER_START` گزارش می‌کند.

</div>

---

<div dir="rtl">

## ۲. مدیریت تسک‌ها

### ۲.۱ ایجاد تسک

</div>

```cpp
// اندازه پشته پیش‌فرض (OS_KERNEL_STACK_SIZE = 512 بایت)
os_task_create(task_function, priority);              // اولویت 1-255
os_task_create(task_function, priority, period_ms);   // + دوره‌ای

// اندازه پشته سفارشی (به بایت)
os_task_create_st(task_function, priority, period_ms, stack_bytes);
```

<div dir="rtl">

**پارامترها:**
| پارامتر | نوع | توضیح |
|----------|------|--------|
| `entry` | `void(*)(void)` | اشاره‌گر تابع تسک (به‌عنوان شناسه یکتا استفاده می‌شود) |
| `priority` | `uint8_t` | 1 = کمترین، 255 = بیشترین. 0 = تسک idle (رزرو شده) |
| `period_ms` | `uint32_t` | 0 = غیردوره‌ای (هر زمان زمان‌بندی شود اجرا می‌شود)، >0 = دوره‌ای |
| `stack_bytes` | `uint32_t` | اندازه پشته به بایت (پیش‌فرض: 512) |

### ۲.۲ شروع و توقف تسک‌ها

</div>

```cpp
os_task_start(task_function);    // فعال‌سازی تسک متوقف‌شده
os_task_stop(task_function);     // غیرفعال‌سازی (حافظه آزاد نمی‌شود)

// بررسی وضعیت
bool running = os_task_isActive(task_function);
uint8_t state = os_task_get_state(task_function); // enum TaskState
```

<div dir="rtl">

> **💡 نکته:** توقف تسک حافظه آن را آزاد نمی‌کند — حافظه به‌صورت ایستا تخصیص یافته است. می‌توانید همان تسک را هر تعداد بار که نیاز دارید شروع/توقف کنید. این مکانیسم اصلی کنترل تسک در زمان اجراست.

### ۲.۳ اولویت تسک

</div>

```cpp
uint8_t prio = os_get_task_priority(task_function);
// اولویت فعال را برمی‌گرداند (ممکن است موقتاً توسط سقف IPC افزایش یافته باشد)
```

<div dir="rtl">

**راهنمای اولویت‌ها:**
- 1: کمترین اولویت برنامه
- 2–4: تسک‌های پس‌زمینه (حسگرها، ثبت‌گر)
- 5–10: تسک‌های عادی (رابط کاربری، ارتباطات)
- 11–20: تسک‌های با اولویت بالا (پایش ایمنی)
- 21+: تسک‌های شبیه ISR (بسیار حیاتی از نظر زمانی)
- 255: رزرو شده (استفاده نکنید)

### ۲.۴ تسک‌های دوره‌ای

</div>

```cpp
// تسک دوره‌ای — در نسخه فعلی، period_ms هنوز شرط اجرای اجباری ایجاد نمی‌کند
void task_sensor(void) {
    while (1) {
        uint16_t adc_val = read_adc();
        // ... پردازش داده ...
        os_yield(); // واگذاری CPU به زمان‌بند
    }
}

// ایجاد با دوره 100 میلی‌ثانیه
os_task_create(task_sensor, 3, 100);
```

<div dir="rtl">

> **💡 نکته:** برای تسک‌های دوره‌ای، از `os_delay_ms()` در انتهای هر تکرار استفاده کنید. زمان‌بند به‌طور خودکار تسک را در مرز دوره بعدی بیدار می‌کند. برای توضیح کامل، بخش [۹](#۹-تسک‌های-دوره‌ای--روش-zenos-بدون-تایمر-نرم‌افزاری) را ببینید.

### ۲.۵ راهنمای اندازه پشته

| نوع تسک | پشته پیشنهادی |
|----------|---------------|
| چشمک‌زن LED ساده | 128–256 بایت |
| خواندن حسگر (HAL) | 256–512 بایت |
| ارتباط UART | 512–1024 بایت |
| SPI/I2C + پشته پروتکل | 512–1024 بایت |
| printf / فرمت‌بندی اعشاری | 1024–2048 بایت |
| منطق برنامه پیچیده | 1024–2048 بایت |

> **💡 نکته:** از `os_get_stack_usage(task)` در زمان توسعه استفاده کنید تا مصرف واقعی اوج را اندازه بگیرید، سپس پشته را ≥130% اوج تنظیم کنید.

</div>

---

<div dir="rtl">

## ۳. زمان‌بندی و تأخیرها

### ۳.۱ تأخیر میلی‌ثانیه‌ای

</div>

```cpp
os_delay_ms(100);  // خواب به مدت ~100ms، واگذاری CPU به تسک‌های دیگر
os_delay_ms(0);    // واگذاری فوری (به تسک‌های دیگر فرصت اجرا می‌دهد)
```

<div dir="rtl">

### ۳.۲ تأخیر میکروثانیه‌ای (انتظار شلوغ)

</div>

```cpp
os_delay_us(10);    // انتظار شلوغ ~10µs (CPU را بلاک می‌کند — هیچ تسک دیگری اجرا نمی‌شود!)
os_delay_us(1000);  // انتظار شلوغ ~1ms
```

<div dir="rtl">

> **⚠️ هشدار:** `os_delay_us()` از شمارنده چرخه DWT برای انتظار شلوغ استفاده می‌کند. CPU را واگذار **نمی‌کند**. فقط برای تأخیرهای بسیار کوتاه (< 1ms) استفاده کنید. برای تأخیرهای طولانی‌تر از `os_delay_ms()` استفاده کنید.

### ۳.۳ دریافت زمان جاری

</div>

```cpp
uint32_t ticks = os_get_tick();  // شمارنده خام تیک
uint32_t ms    = os_get_ms();    // میلی‌ثانیه از زمان بوت
uint32_t us    = os_get_us();    // میکروثانیه از زمان بوت (شمارنده چرخه DWT)
```

<div dir="rtl">

### ۳.۴ الگوهای زمانی

**اندازه‌گیری زمان سپری‌شده:**

</div>

```cpp
uint32_t t0 = os_get_ms();
// ... انجام کار ...
uint32_t elapsed = os_get_ms() - t0;  // حتی اگر ms بیش‌تر شود کار می‌کند (~49 روز)
```

<div dir="rtl">

**بررسی دوره‌ای بدون بلاک:**

</div>

```cpp
static uint32_t last_check = 0;
if (os_get_ms() - last_check >= 100) {
    last_check = os_get_ms();
    // ... انجام کار دوره‌ای ...
}
```

<div dir="rtl">

> **💡 نکته:** همیشه از الگوی تفریق `os_get_ms() - last >= interval` به‌جای مقایسه مطلق استفاده کنید. این روش درست شمارنده میلی‌ثانیه 32 بیتی را مدیریت می‌کند.

</div>

---

<div dir="rtl">

## ۴. بخش‌های حیاتی (OS_SAFE)

### ۴.۱ استفاده پایه

</div>

```cpp
volatile uint32_t shared_counter = 0;

// در تسک A:
OS_SAFE {
    shared_counter++;  // در اینجا وقفه‌ها غیرفعال هستند
}
// در اینجا وقفه‌ها دوباره فعال می‌شوند
```

<div dir="rtl">

`OS_SAFE` تمام وقفه‌ها را در مدت بلاک غیرفعال و سپس دوباره فعال می‌کند. این **تنها راه امن** برای محافظت از متغیرهای مشترک بین تسک‌ها و ISRهاست.

### ۴.۲ کاری که OS_SAFE انجام می‌دهد

</div>

```
┌─ OS_SAFE { ─────────────────────────┐
│  1. غیرفعال‌سازی وقفه‌ها (CPSID I)   │
│  2. ورود به بلاک محافظت‌شده          │
│  3. فعال‌سازی وقفه‌ها (CPSIE I)       │
└──────────────────────────────────────┘
```

<div dir="rtl">

پیاده‌سازی از RAII (کلاس C++ `_OsSafeGuard`) استفاده می‌کند — وقفه‌ها همیشه دوباره فعال می‌شوند، حتی اگر استثنا رخ دهد.

### ۴.۳ تو در تو

</div>

```cpp
OS_SAFE {
    shared_counter++;
    OS_SAFE {
        // تو در تو — عمق ردیابی می‌شود، وضعیت وقفه حفظ می‌شود
        another_var++;
    }
    // هنوز در OS_SAFE بیرونی — وقفه‌ها هنوز غیرفعال‌اند
}
// در اینجا وقفه‌ها دوباره فعال می‌شوند
```

<div dir="rtl">

### ۴.۴ پایش مدت زمان

اگر `OS_SAFETY_MAX_CRITICAL_US` پیکربندی شده باشد (پیش‌فرض 1000µs)، بخش حیاتی که از حد فراتر رود خطای `SAFE_TOO_LONG` ایجاد می‌کند.

**بخش‌های حیاتی را کوتاه نگه دارید!**

</div>

```cpp
// ✅ خوب — بخش حیاتی کوتاه
OS_SAFE { shared_counter++; }

// ❌ بد — بخش حیاتی طولانی، تمام وقفه‌ها را بلاک می‌کند
OS_SAFE {
    HAL_UART_Transmit(&huart1, data, len, 1000);  // حدود 10ms بلاک می‌کند!
}
```

<div dir="rtl">

### ۴.۵ OS_SAFE در مقابل OS_LOCK

| ویژگی | `OS_SAFE` | `OS_LOCK` |
|--------|----------|----------|
| وقفه‌ها را غیرفعال می‌کند | ✅ بله | ❌ خیر |
| بلاک مجاز است | ❌ خیر (خطا می‌دهد) | ✅ بله |
| از ISRها محافظت می‌کند | ✅ بله | ❌ خیر |
| از تسک‌های دیگر محافظت می‌کند | ✅ بله | ✅ بله |
| مورد استفاده | اشتراک‌گذاری داده با ISRs | محافظت از منابع مشترک بین تسک‌ها |

> **💡 نکته:** فقط زمانی از `OS_SAFE` استفاده کنید که نیاز دارید داده را با توابع وقفه به اشتراک بگذارید. برای همگام‌سازی بین تسک‌ها، ترجیحاً از `OS_LOCK` (قفل) استفاده کنید — وقفه‌ها را بلاک نمی‌کند.

</div>

---

<div dir="rtl">

## ۵. eventها (OS_EVENT)

### ۵.۱ سیگنال‌دهی پایه

</div>

```cpp
OS_EVENT sensor_ready;

// تسک A (منتظر):
if (sensor_ready.wait(1000)) {  // حداکثر 1000ms منتظر بمان
    // event سیگنال شد
    process_sensor_data();
} else {
    // تایم‌اوت — event به‌موقع سیگنال نشد
    handle_timeout();
}

// تسک B (سیگنال‌دهنده):
read_sensor();
sensor_ready.signal();  // منتظر را بیدار کن
```

<div dir="rtl">

### ۵.۲ سیگنال‌دهی ISR به تسک

</div>

```cpp
OS_KEY_PRESS;

// در ISR EXTI:
void EXTI0_IRQHandler(void) {
    OS_KEY_PRESS.signal_from_isr();  // نسخه امن برای ISR
    __HAL_GPIO_EXTI_CLEAR_PIN(...);
}

// در تسک:
void task_handle_input(void) {
    while (1) {
        if (OS_KEY_PRESS.wait(5000)) {
            // کلید فشرده شد
        }
    }
}
```

<div dir="rtl">

> **💡 نکته:** همیشه در توابع وقفه از `signal_from_isr()` استفاده کنید. فراخوانی `signal()` از ISR رفتار تعریف‌نشده است.

### ۵.۳ انباشت سیگنال

</div>

```cpp
OS_EVENT evt;

evt.signal();  // شمارنده = 1
evt.signal();  // شمارنده = 2
evt.signal();  // شمارنده = 3

evt.wait();    // شمارنده = 2 (مقدار 1 برمی‌گرداند)
evt.wait();    // شمارنده = 1 (مقدار 1 برمی‌گرداند)
evt.wait();    // شمارنده = 0 (مقدار 1 برمی‌گرداند)
evt.wait();    // مقدار 0 برمی‌گرداند (تایم‌اوت — شمارنده قبلاً 0 بود)
```

<div dir="rtl">

### ۵.۴ انتظار برای چندین event

</div>

```cpp
OS_EVENT evt_a, evt_b;

// انتظار برای هر کدام (هر کدام زودتر بیاید)
uint32_t mask = evt_a | evt_b;  // فقط برای شناسه‌های 0-31 کار می‌کند
// از os_event_signal_from_isr(mask) برای سیگنال‌دهی ISR استفاده کنید

// انتظار فردی با تایم‌اوت کوتاه به‌عنوان polling
while (1) {
    if (evt_a.wait(10) == 1) {
        handle_a();
    }
    if (evt_b.wait(10) == 1) {
        handle_b();
    }
}
```

<div dir="rtl">

### ۵.۵ محدودیت‌های شناسه event

- eventها شناسه‌های خودکار از 0 شروع می‌شوند
- شناسه‌های 0–31: پشتیبانی کامل (سیگنال‌دهی مبتنی بر ماسک، عملگر `|`)
- شناسه‌های ≥ 32: پشتیبانی می‌شوند اما باید به‌صورت جداگانه سیگنال داده شوند (بدون ماسک)
- محدودیت عملی: صدها event در هر سیستم

</div>

---

<div dir="rtl">

## ۶. قفل‌ها (OS_MUTEX)

### ۶.۱ استفاده پایه

</div>

```cpp
OS_MUTEX uart_mtx;  // قفل سراسری

// تسک A:
OS_LOCK(uart_mtx) {
    HAL_UART_Transmit(&huart1, data_a, len, 100);
}  // به‌طور خودکار در پایان بلاک باز می‌شود

// تسک B:
OS_LOCK(uart_mtx) {
    HAL_UART_Transmit(&huart1, data_b, len, 100);
}
```

<div dir="rtl">

`OS_LOCK` یک محافظ قفل RAII است — در ورود قفل را به دست می‌آورد و در خروج (شامل برگشت‌های زودهنگام و استثناها) آن را آزاد می‌کند.

### ۶.۲ قفل/باز کردن دستی

</div>

```cpp
OS_MUTEX mtx;

if (mtx.lock(1000)) {  // حداکثر 1000ms منتظر بمان
    // قفل به دست آمد
    do_work();
    mtx.unlock();
} else {
    // تایم‌اوت
}
```

<div dir="rtl">

### ۶.۳ قفل بازگشتی

</div>

```cpp
OS_MUTEX mtx;

OS_LOCK(mtx) {
    OS_LOCK(mtx) {  // بازگشتی — همان تسک می‌تواند دوباره قفل کند
        OS_LOCK(mtx) {  // تو در تو سه‌گانه هم کار می‌کند
            // هنوز محافظت شده
        }
    }
}  // هر سه سطح باز می‌شوند
```

<div dir="rtl">

### ۶.۴ سقف IPC (ارث‌بری اولویت)

سقف IPC از وارونگی اولویت جلوگیری می‌کند — یک مشکل ایمنی حیاتی در سیستم‌های بلادرنگ.

</div>

```cpp
// اولویت‌های تسک: sensor=2, controller=5, safety=8

// قفل با سقف 8 (اولویت بالاترین کاربر)
OS_MUTEX shared_data_mtx(8);

// sensor (اولویت 2) قفل را به دست می‌آورد → موقتاً به اولویت 8 افزایش می‌یابد
// safety (اولویت 8) منتظر قفل می‌ماند → sensor افزایش می‌یابد، فوراً اجرا می‌شود
// controller (اولویت 5) نمی‌تواند پیشی بگیرد → وارونگی نیست!
```

<div dir="rtl">

**نحوه عملکرد:**
1. وقتی تسکی قفل را به دست می‌آورد، اولویتش به سقف افزایش می‌یابد
2. سقف باید روی اولویت بالاترین تسکی که قفل را به دست می‌آورد تنظیم شود
3. در باز کردن، اولویت به مقدار اصلی (پایه) برمی‌گردد

</div>

```cpp
// پیکربندی صحیح سقف
OS_MUTEX sensor_mtx(8);  // سقف = 8 (اولویت تسک safety)
// sensor(2)، controller(5)، safety(8) همه از این قفل استفاده می‌کنند
// وقتی sensor آن را نگه دارد، sensor با اولویت 8 اجرا می‌شود
// safety هرگز منتظر controller نمی‌ماند — وارونگی جلوگیری شد
```

<div dir="rtl">

> **💡 نکته:** سقف را روی اولویت **بالاترین تسکی** که هرگز قفل را به دست می‌آورد تنظیم کنید. سقف نادرست می‌تواند باعث وارونگی اولویت یا تضاد اولویت شود.

### ۶.۵ قوانین ایمنی قفل

| قانون | نتیجه نقض |
|--------|-----------|
| هر قفلی را که می‌گیرید باز کنید | قفل همیشه بسته می‌ماند، تسک‌های دیگر deadlock می‌کنند |
| قفل/باز کردن در همان scope | خطر فراموش کردن باز کردن |
| هرگز از ISR قفل نکنید | تا ابد بلاک می‌شود — ISR نمی‌تواند واگذاری کند |
| هرگز `OS_SAFE` + `OS_LOCK` را با هم استفاده نکنید | `OS_SAFE` وقفه‌ها را غیرفعال می‌کند، `OS_LOCK` به آن‌ها نیاز دارد |
| تعداد قفل و باز کردن برابر باشد | باز کردن مضاعف تحمل می‌شود اما شمارنده خطا افزایش می‌یابد |

> **💡 نکته:** همیشه `OS_LOCK(mtx) { ... }` را به‌جای `lock()`/`unlock()` دستی ترجیح دهید. محافظ RAII تضمین می‌کند که قفل همیشه آزاد شود، حتی اگر زود برگردید یا استثنا رخ دهد.

</div>

---

<div dir="rtl">

## ۷. صفوف (OS_QUEUE)

### ۷.۱ استفاده پایه

</div>

```cpp
OS_QUEUE<int, 8> sensor_queue;  // ظرفیت: 8 آیتم از نوع int

// تسک تولیدکننده:
sensor_queue.put(42);          // اگر پر باشد بلاک می‌کند
sensor_queue.put(100, 100);    // حداکثر 100ms منتظر، اگر هنوز پر باشد false برمی‌گرداند

// تسک مصرف‌کننده:
int value;
if (sensor_queue.get(value, 1000)) {  // حداکثر 1000ms منتظر بمان
    process(value);
}
```

<div dir="rtl">

### ۷.۲ عملیات صف امن ISR

</div>

```cpp
OS_QUEUE<uint16_t, 32> adc_queue;

// در ISR ADC:
void ADC_IRQHandler(void) {
    uint16_t val = HAL_ADC_GetValue(&hadc1);
    adc_queue.put_from_isr(val);  // بدون بلاک، اگر پر باشد false برمی‌گرداند
}

// در تسک:
void task_process_adc(void) {
    while (1) {
        uint16_t val;
        if (adc_queue.get(val, 100)) {
            // پردازش مقدار ADC
        }
    }
}
```

<div dir="rtl">

> **💡 نکته:** `put_from_isr()` بدون بلاک است — اگر صف پر باشد فوراً `false` برمی‌گرداند. این روش از هر ISR بدون غیرفعال‌سازی وقفه‌ها امن است.

### ۷.۳ پرس‌وجوی وضعیت صف

</div>

```cpp
OS_QUEUE<int, 16> q;

q.put(1); q.put(2); q.put(3);

q.get_count();     // 3
q.get_capacity();  // 16
q.is_full();       // false
q.is_empty();      // false

q.reset();         // پاک کردن همه آیتم‌ها
q.is_empty();      // true
q.get_count();     // 0
```

<div dir="rtl">

### ۷.۴ صفوف تایپ‌شده

</div>

```cpp
// صف ساختارها
struct SensorData {
    uint16_t temperature;
    uint16_t humidity;
    uint32_t timestamp;
};

OS_QUEUE<SensorData, 16> sensor_data_queue;

// صف اشاره‌گرها (برای داده با اندازه متغیر)
OS_QUEUE<void*, 8> msg_queue;
```

<div dir="rtl">

> **💡 نکته:** `OS_QUEUE` یک قالب (template) است — ظرفیت در زمان کامپایل ثابت است. ظرفیت را بر اساس سناریوهای بدترین حالت انفجار انتخاب کنید. خیلی کوچک = تولیدکننده بلاک می‌شود؛ خیلی بزرگ = RAM هدر می‌رود.

</div>

---

<div dir="rtl">

## ۸. سیگنال‌ها (OS_SEMAPHORE)

### ۸.۱ سیگنال دودویی (قفل منبع)

</div>

```cpp
OS_SEMAPHORE spi_sem(1);  // مقدار اولیه = 1 (یک منبع)

// تسک A:
spi_sem.wait();   // به دست آوردن (شمارنده → 0)
SPI_Transmit(data);
spi_sem.signal(); // آزاد کردن (شمارنده → 1)

// تسک B:
spi_sem.wait();   // اگر A نگه داشته باشد منتظر می‌ماند
SPI_Transmit(data2);
spi_sem.signal();
```

<div dir="rtl">

### ۸.۲ سیگنال شمارشی (مجموعه منابع)

</div>

```cpp
OS_SEMAPHORE pool_sem(3);  // 3 منبع موجود

pool_sem.wait();  // شمارنده = 2
pool_sem.wait();  // شمارنده = 1
pool_sem.wait();  // شمارنده = 0
pool_sem.wait(100); // تایم‌اوت — منبعی موجود نیست

pool_sem.signal();  // شمارنده = 1
pool_sem.signal();  // شمارنده = 2
```

<div dir="rtl">

### ۸.۳ سیگنال محدود

</div>

```cpp
OS_SEMAPHORE bounded_sem(0, 5);  // اولیه 0، حداکثر 5

bounded_sem.signal();  // شمارنده = 1
bounded_sem.signal();  // شمارنده = 2
bounded_sem.signal();  // شمارنده = 3
bounded_sem.signal();  // شمارنده = 4
bounded_sem.signal();  // شمارنده = 5
bounded_sem.signal();  // false برمی‌گرداند — حد نرسیده!
```

<div dir="rtl">

### ۸.۴ سیگنال‌دهی امن ISR

</div>

```cpp
OS_SEMAPHORE data_ready(0);

// در ISR:
void TIM2_IRQHandler(void) {
    data_ready.signal_from_isr();  // امن برای ISR
}

// در تسک:
void task_process(void) {
    while (1) {
        if (data_ready.wait(1000)) {
            process_data();
        }
    }
}
```

---

<div dir="rtl">

## ۹. تسک‌های دوره‌ای — روش ZenOS

### ۹.۱ تایمر نرم‌افزاری در برابر تسک دوره‌ای

ZenOS یک API جداگانه برای تایمر نرم‌افزاری ارائه نمی‌کند. در API ساخت تسک، آرگومان اختیاری `period_ms` وجود دارد و هسته مقدار متناظر آن را در فیلدهای `period_ticks` و `next_run_time` در TCB نگهداری می‌کند.

**نکته مهم درباره نسخه فعلی هسته:** در مسیر PendSV این فیلدها به‌روزرسانی می‌شوند، اما تابع `os_pq_next()` هنوز از `next_run_time` برای فیلترکردن صف آماده استفاده نمی‌کند. بنابراین، در وضعیت فعلی نباید `period_ms` را به‌عنوان تضمین اجرای خودکار با نرخ ثابت مستند کرد.

وقتی هدف مثال، نشان‌دادن نقطه زمان‌بندی است، از `os_yield()` استفاده کنید:

```cpp
void task_read_sensor(void) {
    while (1) {
        uint16_t val = read_adc();
        send_to_queue(val);

        // به یک تسک آماده دیگر فرصت اجرا می‌دهد.
        os_yield();
    }
}

os_task_create(task_read_sensor, 3, 100);
```

در این مثال، `os_yield()` یعنی **واگذاری CPU**، نه «خوابیدن به مدت ۱۰۰ میلی‌ثانیه». مقدار `100` به‌عنوان دوره در TCB ذخیره می‌شود، اما اجرای واقعاً دوره‌ای و ثابت، به تکمیل شرط دوره در زمان‌بند هسته نیاز دارد.

### ۹.۲ تفاوت `os_yield()` و تأخیر

هر primitive را مطابق معنای واقعی خودش استفاده کنید:

```cpp
os_yield();        // زمان‌بندی مجدد، بدون خواباندن عمدی تسک.
os_delay_ms(100);  // مسدودکردن تسک فعلی برای حدود ۱۰۰ میلی‌ثانیه.
os_delay_us(10);   // انتظار مشغول برای مدت کوتاه؛ CPU واگذار نمی‌شود.
```

برای شبیه‌سازی یک تسک دوره‌ای، از `os_delay_ms(1..10)` به‌عنوان جایگزین مصنوعی زمان‌بندی دوره‌ای استفاده نکنید. چنین الگویی مفهوم مثال را مبهم می‌کند و رفتار تسک را به یک مقدار تأخیر نامرتبط وابسته می‌سازد.

### ۹.۳ داده‌هایی که هسته فعلی برای دوره نگهداری می‌کند

ساختار TCB شامل این موارد است:

- `period_ticks` — دوره تنظیم‌شده، تبدیل‌شده به تیک هسته.
- `next_run_time` — زمان اجرای بعدی که در مسیر PendSV ثبت می‌شود.
- `last_yield_tick` — برای پایش و منطق watchdog استفاده می‌شود.

بنابراین، پیاده‌سازی فعلی زیرساخت ثبت زمان دوره‌ای را دارد، اما انتخاب صف آماده هنوز باید `next_run_time` را اعمال کند. تا قبل از آن، نباید این قابلیت را به‌عنوان «تسک دوره‌ای کاملاً خودکار» معرفی کرد.

### ۹.۴ الگوی مورد نظر پس از تکمیل شرط دوره در زمان‌بند

پس از تکمیل شرط دوره در scheduler، الگوی پیشنهادی برای مثال‌ها این است:

```cpp
void task_sensor(void) {
    while (1) {
        uint16_t val = read_adc();
        sensor_queue.put(val, 100);

        // تسک دوره‌ای: واگذاری CPU، نه شبیه‌سازی دوره با os_delay_ms().
        os_yield();
    }
}

os_task_create(task_sensor, 3, 100);
```

این قرارداد در تمام مستندات ZenOS یکسان خواهد بود:

**تسک دوره‌ای → `os_yield()`، خواب یا تأخیر واقعی → `os_delay_ms()`**


## ۱۰. مدیریت خطا و پایش

### ۱۰.۱ گزارش‌دهی خطا

</div>

```cpp
// گزارش خطای سفارشی
os_log_error(OSError::SENSOR_TIMEOUT, (uint8_t)ErrorSeverity::WARNING);

// بررسی شمارنده‌های خطا
uint32_t total      = os_get_error_count();
uint32_t unexpected = os_get_unexpected_error_count();
uint32_t expected   = os_get_expected_error_count();

OSError last = os_get_last_error();
```

<div dir="rtl">

### ۱۰.۲ سرکوب خطاهای مورد انتظار

در زمان تست، ممکن است عمداً خطا ایجاد کنید. از `OS_ERROR_EXPECTED` استفاده کنید تا آن‌ها را از شمارنده غیرمنتظره حذف کنید:

</div>

```cpp
// در کد تست:
OS_ERROR_EXPECTED {
    os_event_signal(-1);  // عمداً نامعتبر — خطای مورد انتظار
}
// os_get_unexpected_error_count() افزایش نمی‌یابد
```

<div dir="rtl">

### ۱۰.۳ ثبت خطا

</div>

```cpp
#if OS_MONITOR_ERROR_LOG
    // ثبت خطا
    os_log_error(OSError::STACK_OVERFLOW, (uint8_t)ErrorSeverity::CRITICAL);

    // خواندن ورودی‌های ثبت (جدیدترین ابتدا)
    uint32_t count = os_get_error_log_count();
    for (uint32_t i = 0; i < count; i++) {
        OSErrorEntry entry = os_get_error_log_entry(i);
        // entry.timestamp_tick — چه زمانی رخ داد
        // entry.code          — کدام خطا
        // entry.task_id       — کدام تسک
        // entry.severity      — INFO, WARNING, CRITICAL
    }
#endif
```

<div dir="rtl">

### ۱۰.۴ پایش مصرف پشته

</div>

```cpp
#if OS_MONITOR_ENABLED
    // مصرف پشته هر تسک
    uint32_t bytes = os_get_stack_usage(task_function);

    // گزارش کامل
    uint8_t count = os_get_stack_report_count();
    for (uint8_t i = 0; i < count; i++) {
        os_stack_report_entry_t rep;
        os_get_stack_report(i, &rep);
        // rep.name       — نام تسک
        // rep.size_bytes — اندازه کل پشته
        // rep.peak_bytes — مصرف اوج از آخرین شروع/ریست
    }

    // مصرف CPU
    uint8_t total_cpu = os_get_cpu_usage_total();
    uint8_t task_cpu  = os_get_task_cpu_usage(task_function);
#endif
```

<div dir="rtl">

### ۱۰.۵ پایش ددلاین

</div>

```cpp
#if OS_MONITOR_DEADLINE
    // تنظیم ددلاین برای تسک (باید بعد از ایجاد فراخوانی شود)
    os_task_set_deadline(task_safety, 50);  // ددلاین 50ms

    // بررسی تأخیرها
    uint32_t misses = os_get_deadline_miss_count(task_safety);
    if (misses > 0) {
        // مدیریت تأخیر ددلاین (ثبت، ریست و غیره)
    }
#endif
```

---

<div dir="rtl">

## ۱۱. ویژگی‌های ایمنی

### ۱۱.۱ تایمر سخت‌افزاری

</div>

```cpp
#if OS_SAFETY_HW_WATCHDOG
    // در یک تسک اختصاصی:
    void task_wdg_feeder(void) {
        while (1) {
            os_hw_watchdog_feed();  // تغذیه بدون قید و شرط
            os_delay_ms(1000);     // هر 1 ثانیه تغذیه ( timeouts ~6.5s)
        }
    }

    // یا تغذیه مشروط (اگر ناسالم باشد رد کن):
    os_hw_watchdog_check();  // فقط اگر error_count < 10 باشد تغذیه می‌کند
#endif
```

<div dir="rtl">

### ۱۱.۲ آزمون RAM (پس‌زمینه)

</div>

```cpp
#if OS_SAFETY_RAM_TEST
    // از تسک idle یا تسک پس‌زمینه کم‌اولویت فراخوانی کنید
    void task_background(void) {
        while (1) {
            os_ram_test_step();  // هر فراخوانی یک کلمه را آزمایش می‌کند
            
            if (os_ram_test_complete()) {
                if (os_get_ram_test_error_count() > 0) {
                    // خرابی SRAM شناسایی شد!
                    os_log_error(OSError::RAM_TEST_FAIL, 2);
                }
                // پس از تکمیل، فراخوانی os_ram_test_step() ریست و از نو شروع می‌کند
            }
            
            os_yield();  // به تسک‌های دیگر اجازه اجرا بده
        }
    }
#endif
```

<div dir="rtl">

### ۱۱.۳ آزمون CRC (یکپارچگی ROM)

</div>

```cpp
#if OS_SAFETY_CRC_CHECK
    // در زمان بوت (قبل از os_start):
    os_crc_init();  // محاسبه CRC مورد انتظار روی فلش

    // در تسک پس‌زمینه:
    void task_crc_check(void) {
        while (1) {
            os_crc_check_step();  // هر فراخوانی 64 کلمه را بررسی می‌کند
            
            if (os_crc_check_complete()) {
                if (os_get_crc_error_count() > 0) {
                    // خرابی ROM شناسایی شد!
                    os_log_error(OSError::HARDFAULT, 2);
                    // در نظر بگیرید یک ریست کنترل‌شده ایجاد کنید
                }
            }
            
            os_delay_ms(100);  // هر 100ms اجرا شود
        }
    }
#endif
```

<div dir="rtl">

### ۱۱.۴ محافظت MPU

</div>

```cpp
#if OS_SAFETY_MPU
    // راه‌اندازی MPU در زمان بوت (قبل از os_start):
    os_mpu_init();
    os_mpu_enable();

    // اضافه کردن مناطق حافظه اختصاصی هر تسک:
    extern TCB task_sensor_tcb;
    os_mpu_add_region(&task_sensor_tcb,
        0x20001000,   // آدرس پایه
        4096,          // اندازه (4KB)
        3              // AP: دسترسی کامل، غیرقابل اجرا
    );
#endif
```

---

<div dir="rtl">

## ۱۲. نکات و ترفندهای پیشرفته

### ۱۲.۱ مدیریت چرخه حیات تسک

</div>

```cpp
// ایجاد تمام تسک‌ها از قبل، شروع/توقف در صورت نیاز
os_task_create(task_heater, 5);
os_task_create(task_cooler, 5);
os_task_stop(task_heater);   // در ابتدا غیرفعال
os_task_stop(task_cooler);   // در ابتدا غیرفعال

// کنترل در زمان اجرا
if (temperature > 70) {
    os_task_start(task_cooler);
    os_task_stop(task_heater);
} else if (temperature < 20) {
    os_task_start(task_heater);
    os_task_stop(task_cooler);
}
```

<div dir="rtl">

### ۱۲.۲ تخفیف تدریجی

</div>

```cpp
void task_main(void) {
    while (1) {
        // تلاش برای حالت دقت بالا
        if (sensor_available()) {
            read_high_accuracy();
        } else {
            // بازگشت به حالت دقت پایین
            read_low_accuracy();
        }
        
        os_delay_ms(100);
    }
}
```

<div dir="rtl">

### ۱۲.۳ مدیریت توان با بیکاری بدون تیک

</div>

```cpp
// OS_TOOL_TICKLESS_IDLE را در پیکربندی فعال کنید
// سیستم‌عامل از WFI (انتظار برای وقفه) وقتی هیچ تسکی آماده نیست استفاده می‌کند
// این CPU را به‌طور خودکار در حالت کم‌توان قرار می‌دهد

// نکات طراحی برای توان کم:
// 1. به‌جای حلقه‌های انتظار شلوغ از os_delay_ms() استفاده کنید
// 2. تأخیرهای طولانی → خواب عمیق
// 3. تأخیرهای کوتاه → خواب نسبی
// 4. تسک‌های دوره‌ای به‌طور خودکار از WFI بیدار می‌شوند
```

<div dir="rtl">

### ۱۲.۴ نکات اشکال‌زدایی

</div>

```cpp
// 1. چاپ تعداد تسک در زمان شروع
uart_printf("Tasks: %d\n", os_get_task_count());

// 2. پایش مصرف پشته به‌صورت دوره‌ای
uint32_t peak = os_get_stack_usage(task_function);
uart_printf("Stack peak: %d bytes\n", peak);

// 3. بررسی مصرف CPU
uint8_t cpu = os_get_task_cpu_usage(task_function);
uart_printf("Task CPU: %d%%\n", cpu);

// 4. چاپ نسخه
uart_printf("ZenOS %s\n", os_get_version_string());

// 5. بررسی خطاهای غیرمنتظره
if (os_get_unexpected_error_count() > 0) {
    uart_printf("UNEXPECTED ERRORS: %d\n", os_get_unexpected_error_count());
}
```

<div dir="rtl">

### ۱۲.۵ نکات بهینه‌سازی کامپایلر

</div>

```cpp
// داده‌های بحرانی عملکرد را اگر از ISRها دسترسی دارند volatile علامت‌گذاری کنید
static volatile uint32_t isr_counter = 0;

// برای به‌روزرسانی کوتاه متغیر مشترک از OS_SAFE استفاده کنید
OS_SAFE { isr_counter++; }

// برای انتقال داده طولانی ISR به تسک، به‌جای آن از صف استفاده کنید
OS_QUEUE<uint16_t, 32> isr_data_queue;
// در ISR: isr_data_queue.put_from_isr(val);
// در تسک: isr_data_queue.get(val, 100);
```

---

<div dir="rtl">

## ۱۳. کتابخانه الگوهای رایج

### ۱۳.۱ ماشین حالت در یک تسک

</div>

```cpp
enum State { INIT, RUNNING, ERROR, SLEEP };
static State state = INIT;

void task_state_machine(void) {
    while (1) {
        switch (state) {
            case INIT:
                if (hardware_ready()) state = RUNNING;
                break;
            case RUNNING:
                if (error_detected()) state = ERROR;
                else do_work();
                break;
            case ERROR:
                if (error_cleared()) state = RUNNING;
                else enter_safe_state();
                break;
            case SLEEP:
                os_delay_ms(1000);
                break;
        }
        os_yield();        // همیشه CPU را واگذار کنید
    }
}
```

<div dir="rtl">

### ۱۳.۲ مدیریت دکمه با حذف نویز

</div>

```cpp
OS_KEY_EVT;

void EXTI0_IRQHandler(void) {
    OS_KEY_EVT.signal_from_isr();
}

void task_button(void) {
    while (1) {
        if (OS_KEY_EVT.wait(5000)) {
            os_delay_ms(50);  // تأخیر حذف نویز
            if (read_button()) {
                handle_button_press();
            }
        }
    }
}
```

<div dir="rtl">

### ۱۳.۳ PWM نرم‌افزاری از طریق تسک دوره‌ای

</div>

```cpp
void task_pwm(void) {
    uint8_t duty = 128;  // 50% پرکردگی
    while (1) {
        HAL_GPIO_WritePin(PWM_PORT, PWM_PORT_PIN, GPIO_PIN_SET);
        os_delay_us(duty * 10);  // زمان روشن
        
        HAL_GPIO_WritePin(PWM_PORT, PWM_PORT_PIN, GPIO_PIN_RESET);
        os_delay_us((255 - duty) * 10);  // زمان خاموش
    }
}

os_task_create(task_pwm, 15);  // اولویت بالا برای زمان‌بندی
```

<div dir="rtl">

### ۱۳.۴ خط تولید حسگر چندمرحله‌ای

</div>

```cpp
OS_QUEUE<RawData, 8> raw_queue;
OS_QUEUE<ProcessedData, 8> processed_queue;

void task_acquire(void) {
    while (1) {
        RawData raw = read_sensor();
        raw_queue.put(raw);
        os_yield();
    }
}

void task_process(void) {
    while (1) {
        RawData raw;
        if (raw_queue.get(raw, 100)) {
            ProcessedData proc = filter_and_calibrate(raw);
            processed_queue.put(proc);
        }
    }
}

void task_output(void) {
    while (1) {
        ProcessedData proc;
        if (processed_queue.get(proc, 100)) {
            display(proc);
        }
    }
}

// خط تولید: acquire(3) → process(5) → output(4)
os_task_create(task_acquire, 3, 10);
os_task_create(task_process, 5);
os_task_create(task_output, 4);
```

<div dir="rtl">

### ۱۳.۵ خاموش‌سازی نرم

</div>

```cpp
static volatile bool shutdown_requested = false;

void task_shutdown_handler(void) {
    while (1) {
        if (shutdown_button.wait(1000)) {
            shutdown_requested = true;
            
            // توقف تسک‌های غیرضروری
            os_task_stop(task_ui);
            os_task_stop(task_led);
            
            // ذخیره وضعیت در فلش
            save_critical_data();
            
            // ورود به حالت ایمن
            enter_safe_mode();
        }
    }
}
```

---

<div dir="rtl">

## پیوست: مرجع سریع API

### توابع هسته (C)

| تابع | توضیح |
|------|--------|
| `os_init()` | راه‌اندازی سیستم‌عامل (قبل از ایجاد تسک فراخوانی شود) |
| `os_start()` | شروع زمان‌بند (هرگز برنمی‌گردد) |
| `os_delay_ms(ms)` | خواب به مدت `ms` میلی‌ثانیه |
| `os_delay_us(us)` | انتظار شلوغ به مدت `us` میکروثانیه |
| `os_yield()` | واگذاری به تسک آماده بعدی |
| `os_get_tick()` | دریافت شمارنده خام تیک |
| `os_get_ms()` | دریافت میلی‌ثانیه از زمان بوت |
| `os_get_us()` | دریافت میکروثانیه از زمان بوت |
| `os_task_start(entry)` | شروع تسک متوقف‌شده |
| `os_task_stop(entry)` | توقف تسک در حال اجرا |
| `os_task_isActive(entry)` | بررسی فعال بودن تسک |
| `os_get_task_count()` | دریافت تعداد تسک‌های ثبت‌شده |
| `os_get_task_priority(entry)` | دریافت اولویت فعال تسک |

### کلاس‌های RAII (C++)

| کلاس | ماکرو | توضیح |
|-------|--------|--------|
| `_OsSafeGuard` | `OS_SAFE { ... }` | محافظ غیرفعال‌سازی وقفه |
| `_OsLockGuard` | `OS_LOCK(mtx) { ... }` | محافظ قفل mutex |
| `OS_EVENT` | — | سیگنال‌دهی event بین تسک‌ها |
| `OS_MUTEX` | — | قفل با ارث‌بری اولویت |
| `OS_QUEUE<T،N>` | — | صف FIFO محدود |
| `OS_SEMAPHORE` | — | سیگنال شمارشی |

</div>

---<div dir="rtl"> *تولید شده برای ZenOS RTOS v1.0.1 — همچنین ببینید: CONFIG_GUIDE_FA.md، SAFETY_MANUAL.md، ZenOS_Config.hpp*</div>
