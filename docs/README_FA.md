<div align="center" dir="rtl">

<img src="guide-html/ZenOS_logo.svg" alt="ZenOS Logo" width="400" />

**سیستم‌عامل بلادرنگ برای ARM Cortex-M — نوشته‌شده با C++11.**

### **سادگی — امنیت — سرعت**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg?style=for-the-badge)](../LICENSE)
[![Version](https://img.shields.io/badge/Version-1.1.0-green.svg?style=for-the-badge)]()
[![Platform](https://img.shields.io/badge/Platform-ARM%20Cortex--M-orange.svg?style=for-the-badge)]()
[![Language](https://img.shields.io/badge/Language-C%2B%2B11-purple.svg?style=for-the-badge)]()
[![Standard](https://img.shields.io/badge/IEC-62304-red.svg?style=for-the-badge)]()
[![Standard](https://img.shields.io/badge/IEC-61508-red.svg?style=for-the-badge)]()

<br>

[**🇮🇷 فارسی**](README_FA.md) | [**🇬🇧 English**](../README.md)

📘 **[راهنمای ZenOS](https://raymon-research-team.github.io/ZenOS-RTOS/)**

<br>

</div>

---

<div dir="rtl">

## 🧘 درباره پروژه


کلمه *Zen* به معنای شفافیت از طریق سادگی است — کنار گذاشتن زواید تا فقط آنچه اهمیت دارد باقی بماند. ایده پشت این سیستم‌عامل همین است.

ZenOS کارهای سخت سیستم‌های تعبیه‌شده — زمان‌بندی، همگام‌سازی، حفاظت حافظه، بازیابی خطا — را خودش انجام می‌دهد تا شما روی برنامه‌تان تمرکز کنید. چیزهایی که معمولاً ده‌ها خط کد راه‌اندازی می‌خواهند اینجا به یک فراخوانی تابع خلاصه شده‌اند. سیستم‌عامل خودش بقیه را مدیریت می‌کند.

برای میکروکنترلرهای ARM Cortex-M و پروژه‌های STM32 ساخته شده است (از STM32F0/G0/L0 تا STM32H7/WBA). مکانیسم‌های ایمنی قابل پیکربندی دارد: بررسی پشته، محافظت MPU، watchdog سخت‌افزاری و نرم‌افزاری، تأیید CRC، بررسی یکپارچگی TCB و پایش ددلاین. اگر محصول پزشکی یا صنعتی می‌سازید، می‌توانید بررسی‌های هدف IEC 62304 یا IEC 61508 را در زمان کامپایل فعال کنید — هسته در صورت فعال‌بودن پروفایل، تنظیمات الزامی را با خطای کامپایل بررسی می‌کند.

> **بدون Heap. بدون تخصیص‌های پنهان. بدون سورپرایز.**

</div>

<div dir="rtl">

### ✨ ویژگی‌ها


| ویژگی | جزئیات |
|:------|:-------|
| 🧩 اصول IPC | تسک‌ها، mutex، event، صف، سمافور — همه با حداقل کد اضافه |
| ⬆️ سقف اولویت | پروتکل Immediate Priority Ceiling برای جلوگیری از وارونگی اولویت |
| 🏥 IEC 62304 | بررسی‌های پروفایل هدف در زمان کامپایل |
| 🏭 IEC 61508 | بررسی‌های پروفایل هدف در زمان کامپایل |
| 🧪 تست‌شده | روی سخت‌افزار واقعی (STM32F103C8T6) با مجموعه تست self-test جامع |
| 🔒 صفر هزینه | قالب‌های C++ و RAII — انتزاع بدون هزینه اجرا |

</div>

<div dir="rtl">

### ⚡ چرا ZenOS؟


| مزیت | ZenOS | FreeRTOS | Zephyr |
|:-----|:------|:---------|:-------|
| **تیک** | **۱۰۰ میکروثانیه پیش‌فرض؛ قابل تنظیم ۱۰۰ تا ۱۰۰۰** | قابل تنظیم | قابل تنظیم |
| **زمان‌بند** | **Bitmap اولویت + صف آماده برای هر اولویت + بررسی شرایط اجرا** | زمان‌بند اولویتی | زمان‌بند اولویتی |
| **Heap** | **بدون heap عمومی در مدل تسک هسته** | قابل تنظیم | قابل تنظیم |
| **ایمنی** | **قابل پیکربندی (canary، MPU، watchdog، آزمون RAM، CRC، بررسی TCB)** | قابل تنظیم | قابل تنظیم |
| **IEC 62304/61508** | **بررسی پروفایل هدف در زمان کامپایل** | بدون | بدون |
| **تسک‌های دوره‌ای** | **گیت انتشار (release gate) در زمان‌بند** | مکانیسم‌های برنامه/tایمر | مکانیسم‌های برنامه/tایمر |
| **سقف IPC** | **پشتیبانی سقف اولویت در mutex** | بستگی به پیکربندی/API دارد | بستگی به پیکربندی/API دارد |
| **C++ RAII** | **پشتیبانی کامل (API هسته C++11)** | C/C++ | C/C++ |

> 📖 [مقایسه فنی کامل → ZENOS_ADVANTAGES.md](ZENOS_ADVANTAGES.md)



### 🖥️ خانواده‌های پشتیبانی‌شده

</div>

```
 STM32F1  STM32F2  STM32F4  STM32F7  STM32H7
  M3        M3       M4       M7      Dual M7+M4

 STM32G0  STM32G4  STM32L0  STM32L4  STM32WB
  M0+       M4       M0+       M4       M4
```

---

<div dir="rtl">

## 🚀 شروع سریع

یک برنامه کامل سیستم‌عامل بلادرنگ در کمتر از 10 خط:

**قبل از کد، دو مرحله ضروری:**

> **⚠️ ۱. تایمر HAL را در CubeMX تنظیم کنید:**
> ZenOS از `SysTick` برای تیک هسته خود استفاده می‌کند. اگر HAL هم از `SysTick` استفاده کند، تداخل ایجاد می‌شود. بنابراین باید در CubeMX یک تایمر جداگانه (مثلاً `TIM1`) به عنوان زمان‌بند (timebase) HAL انتخاب کنید. این کار را در مسیر **System Core** → **SYS** → **Timebase Source** = `TIM1` انجام دهید.
>
> **⚠️ ۲. `os_tick()` را به `SysTick_Handler` اضافه کنید:**
> فایل `Src/stm32f1xx_it.c` را باز کنید و `os_tick()` را داخل تابع `SysTick_Handler` فراخوانی کنید. این تنها راه دریافت وقفه تیک توسط ZenOS است:
</div>

<div dir="ltr">

>
> ```c
> #include "ZenOS.hpp"  // در بالای فایل
>
> void SysTick_Handler(void) {
>     os_tick();
> }
> ```
</div>

<div dir="rtl">

>
> **⚠️ ۳. تسک نمونه را مطابق کد زیر به فایل `Src/main.cpp` اضافه کنید:**

</div>

<div dir="ltr">

```cpp
#include "ZenOS.hpp"

void task_blink(void) {
    while (1) {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        os_delay_ms(500);
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    os_init();
    os_task_create(task_blink, 5); // Create task with priority 5
    os_start();
}
```
</div>

<div dir="rtl">

> 💡 همین. زمان‌بند از `os_start()` کنترل را به دست می‌گیرد و هرگز برنمی‌گردد. تسک شما هر 500ms اجرا می‌شود، LED چشمک می‌زند، و شما حتی یک خط کد زمان‌بندی هم ننوشتید.


### 🏗️ معماری

</div>

```
┌───────────────────────────────────────────────────┐
│                 تسک‌های برنامه                     │
├───────────────────────────────────────────────────┤
│             هسته ZenOS (C++11)                    │
│  ┌────────────┬─────────────┬──────────────────┐  │
│  │  زمان‌بند   │     IPC     │  لایه ایمنی       │  │
│  │  Bitmap    │    event    │  canary پشته     │  │
│  │  صف     │   قفل‌ها     │    محافظت MPU       │  │
│  │  اولویت    │    صفوف     │  watchdog نرم/سخت │  │
│  │            │    سمافورها  │     تست RAM      │  │
│  │            │             │    بررسی CRC     │  │
│  │            │             │  پایش ددلاین      │  │
│  └────────────┴─────────────┴──────────────────┘  │
├───────────────────────────────────────────────────┤
│            HAL STM32 (تولیدشده توسط CubeMX)       │
├───────────────────────────────────────────────────┤
│         سخت‌افزار ARM Cortex-M (M3/M4/M7)          │
└───────────────────────────────────────────────────┘
```

<div dir="rtl">

راهنمای کامل API → [API_TUTORIAL_FA.md](API_TUTORIAL_FA.md) | [English](API_TUTORIAL.md)

</div>

---

<div dir="rtl">

## 📚 مستندات


| سند | محتوا |
|:----|:------|
| 🌐 [guide.html](https://raymon-research-team.github.io/ZenOS-RTOS/) | راهنمای تعاملی کامل (HTML) |
| 📖 [SAFETY_MANUAL_FA.md](SAFETY_MANUAL_FA.md) | معماری ایمنی، محدودیت‌ها، جزئیات انطباق با استانداردها |
| ⚙️ [CONFIG_GUIDE_FA.md](CONFIG_GUIDE_FA.md) | راهنمای کامل پیکربندی — همه گزینه‌ها، پیش‌فرض‌ها، پروفایل‌ها، اجرای IEC (فارسی) |
| ⚙️ [CONFIG_GUIDE.md](CONFIG_GUIDE.md) | Complete configuration guide (English) |
| 📘 [API_TUTORIAL_FA.md](API_TUTORIAL_FA.md) | راهنمای کامل API با مثال‌ها (فارسی) |
| 📗 [API_TUTORIAL.md](API_TUTORIAL.md) | راهنمای کامل API با مثال‌ها (انگلیسی) |
| 🏛️ [ARCHITECTURE.md](ARCHITECTURE.md) | ساختار داخلی هسته — زمان‌بند، سوییچ زمینه، مدیریت خطا |
| 📜 [CHANGELOG.md](CHANGELOG.md) | تاریخچه نسخه‌ها |
| ⚡ [ZENOS_ADVANTAGES.md](ZENOS_ADVANTAGES.md) | مقایسه فنی و ویژگی‌های طراحی |
| ⚙️ [ZenOS_Config.hpp](../ZenOS/ZenOS_Config.hpp) | هر گزینه پیکربندی با یادداشت‌های ایمنی |

</div>

---

<div dir="rtl">

## 🛡️ ویژگی‌های ایمنی


مکانیسم‌های ایمنی و پایش **قابل پیکربندی** هستند و در `ZenOS_Config.hpp` تنظیم می‌شوند؛ همه به‌صورت پیش‌فرض فعال نیستند.

| مکانیسم | ماکرو پیکربندی | عملکرد |
|:--------|:---------------|:-------|
| 🔍 canary پشته | همیشه فعال | کلمات canary + بررسی محدوده SP برای هر تسک (هر ۱۶ تیک) |
| 🔐 یکپارچگی TCB | `OS_MONITORING_EN` | اعتبارسنجی عدد جادو برای تشخیص خرابی حافظه |
| ⏱️ پایش ددلاین | `OS_MONITORING_EN` | تشخیص ددلاین؛ واکنش با `OS_MONITORING_DEADLINE_ACTION` |
| 📋 ثبت خطا | `OS_MONITORING_EN` | بافر حلقه‌ای RAM با زمان، شناسه تسک و شدت (`OS_MONITORING_ERROR_LOG_SIZE`) |
| 📊 آمار CPU و پشته | `OS_MONITORING_EN` | بار CPU، واترمارک پشته هر تسک و گزارش پشته |
| 🐕 watchdog سخت‌افزاری | `OS_SAFETY_HW_WATCHDOG_EN` | ادغام IWDG STM32 با تغذیه مشروط |
| 💤 watchdog نرم‌افزاری | `OS_SAFETY_SOFT_WATCHDOG_EN` | تشخیص تسک‌های گیرکرده با `OS_SAFETY_SOFT_WDG_TIMEOUT_MS` |
| 🧪 آزمون RAM | `OS_SAFETY_RAM_TEST_EN` | آزمون March-C پس‌زمینه برای یکپارچگی SRAM |
| ✅ بررسی CRC | `OS_SAFETY_CRC_EN` | یکپارچگی فلش از طریق peripheral CRC |
| 🛡️ MPU | `OS_SAFETY_MPU_EN` | حفاظت حافظه هر تسک (PMSAv7/PMSAv8) در سخت‌افزار پشتیبانی‌شده |

</div>

---

<div dir="rtl">

## 💰 حمایت از ZenOS


این یک پروژه تک‌نفره است. من آن را می‌سازم، تست می‌کنم، مستندات می‌نویسم و نگهداری می‌کنم — همه در اوقات فراغت، بدون بودجه.

> **📝 توجه:** ZenOS پروفایل‌های هدف compile-time برای الزامات پیکربندی IEC 62304 (پزشکی) و IEC 61508 (صنعتی) فراهم می‌کند. این پروفایل‌ها و مکانیسم‌های ایمنی، گواهی رسمی محسوب نمی‌شوند؛ انجام فرایندهای سطح محصول شامل verification، validation، ردیابی، ممیزی و گواهی رسمی بر عهده مالک پروژه/محصول است.

اگر ZenOS برایتان مفید بوده، چه برای یادگیری، چه نمونه‌سازی و چه تولید محصول، از حمایت مالی استقبال می‌کنم.


### 🎯 پول به کجا می‌رود

</div>

<table>
<tr>
<td align="center" width="25%" dir="rtl">

### 🏥 گواهینامه
انطباق IEC 62304 و IEC 61508 ارزان نیست. مستندات رسمی، ردیابی، ممیزی — همه هزینه دارند.

</td>
<td align="center" width="25%" dir="rtl">

### 🔧 سخت‌افزار
بردهای STM32 جدید، تحلیل‌گرهای منطقی، پروب‌های JTAG — هر خانواده به تجهیزات تست خودش نیاز دارد.

</td>
<td align="center" width="25%" dir="rtl">

### ☁️ زیرساخت
خطوط CI، کلاود‌بیلدها، اجرای خودکار تست‌ها. نگهداری همه اینها منابع می‌خواهد.

</td>
<td align="center" width="25%" dir="rtl">

### 📖 مستندات و جامعه
نوشتن مستندات خوب زمان‌بر است. پاسخ به Issues زمان‌بر است. هر دو مهم هستند.

</td>
</tr>
</table>

<div dir="rtl">

## 💸 کمک مالی


حمایت مالی شما مستقیماً توسعه، تست، گواهینامه و مستندات را تأمین می‌کند.

> 💡 **برای کپی کردن آدرس کیف پول، روی خودِ آدرس کلیک کنید. برای باز شدن تصویر QR در تب جدید، روی تصویر کلیک کنید.**

</div>

<br>

<table>
<tr>
<td align="center" width="50%" dir="rtl">

🟠 **بیت‌کوین (BTC)**

[`bc1qd39vgmnweuzh5hp2cqm4cnh782xga6wph3v650`](bitcoin:bc1qd39vgmnweuzh5hp2cqm4cnh782xga6wph3v650)

</td>
<td rowspan="2" align="center" width="50%">

<a href="https://donatr.ee/raymon-research-team/" target="_blank" rel="noopener" title="Open donation page in a new tab"><img src="docs/guide-html/donate-qr.png" alt="Donate QR Code" width="180" /></a>

</td>
</tr>
<tr>
<td align="center" width="50%" dir="rtl">

🔵 **اتریوم (ETH) / تتر (USDT - ERC-20)**

[`0x1C21c39324F65a38Fb8de9ccB92aB01FdeD1534C`](ethereum:0x1C21c39324F65a38Fb8de9ccB92aB01FdeD1534C)

</td>
</tr>
</table>


<div dir="rtl">

> 📩 **پس از انجام پرداخت، لطفاً آدرس ایمیل یا شناسه تراکنش خود را به [rahman.h22@gmail.com](mailto:rahman.h22@gmail.com) ارسال کنید تا شخصاً از شما قدردانی شود.** 


### 🌟 راه‌های دیگر کمک

</div>

<table>

<tr>

<td align="center" width="25%" dir="rtl">⭐<br><b>ستاره</b><br>بدهید</td>
<td align="center" width="25%" dir="rtl">🐛<br><b>باگ</b><br>گزارش کنید</td>
<td align="center" width="25%" dir="rtl">📝<br><b>آموزش</b><br>بنویسید</td>
<td align="center" width="25%" dir="rtl">🗣️<br><b>معرفی</b><br>کنید</td>

</tr>

</table>

---

<div dir="rtl">

## 📬 ارتباط با ما


**تیم تحقیقاتی رایمون** — Rahman Heidari

📧 ایمیل: [rahman.h22@gmail.com](mailto:rahman.h22@gmail.com)

برای سؤالات، پیشنهادات یا فرصت‌های همکاری، با ما در تماس باشید.

</div>

---

<div align="center" dir="rtl">

<br>

**با ❤️ برای جامعه تعبیه‌شده ساخته شده**

[![GitHub stars](https://img.shields.io/github/stars/Raymon-Research-Team/ZenOS-RTOS?style=social)]()

<br>

*ZenOS — (سادگی — امنیت — سرعت)*

</div>
