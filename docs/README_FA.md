<div align="center" dir="rtl">

<img src="guide-html/ZenOS_logo.svg" alt="ZenOS Logo" width="400" />

**سیستم‌عامل بلادرنگ برای ARM Cortex-M — نوشته‌شده با C++11**

### **سادگی · امنیت · سرعت**

[راهنمای ZenOS](https://raymon-research-team.github.io/ZenOS-RTOS/) · [مخزن GitHub](https://github.com/Raymon-Research-Team/ZenOS-RTOS)

</div>

---

<div dir="rtl">

# درباره ZenOS

ZenOS یک سیستم‌عامل بلادرنگ سبک برای میکروکنترلرهای ARM Cortex-M است که تمرکز آن روی زمان‌بندی قابل پیش‌بینی، منابع ایستا، API ساده C++11 و مکانیسم‌های ایمنی قابل پیکربندی است.

هسته امکاناتی مانند مدیریت تسک، زمان‌بندی اولویت‌محور، زمان‌بندی زمانی، event، mutex، صف FIFO محدود، semaphore، پایش مصرف پشته و CPU، ثبت خطا، watchdog، MPU، آزمون RAM و بررسی CRC را فراهم می‌کند.

> **بدون Heap عمومی در مدل تسک‌های هسته؛ منابع ایستا و پیکربندی صریح.**

## اولویت‌ها

اولویت `0` برای تسک idle رزرو شده است. اولویت‌های کاربردی در پیکربندی کامل عبارت‌اند از **`1..255`**؛ هرچه عدد بزرگ‌تر باشد، اولویت بالاتر است.

ماکروی مربوط به تعداد اسلات‌های اولویت:

</div>

```cpp
#define OS_KERNEL_MAX_PRIORITIES 32
```

<div dir="rtl">

مقدار پیش‌فرض ۳۲ اسلات است و محدوده قابل استفاده در آن `1..31` است. هسته می‌تواند تا ۲۵۶ اسلات پیکربندی شود؛ در این حالت محدوده کاربردی `1..255` خواهد بود.

برای اولویت‌های بالاتر از ۳۱، حافظه اضافی موردنیاز bitmap و صف‌های اولویت به‌صورت خودکار از روی مقدار تنظیم‌شده محاسبه می‌شود و هسته به یک جدول ثابت ۲۵۶ تسکی نیاز ندارد.

## ویژگی‌های اصلی

| ویژگی | وضعیت فعلی |
|---|---|
| زمان‌بند | bitmap اولویت + صف مستقل برای هر سطح + بررسی eligibility |
| اولویت کاربردی | **۱ تا ۲۵۵** در پیکربندی ۲۵۶ اسلاتی |
| اسلات پیش‌فرض | ۳۲ سطح (`0..31`) |
| تیک پیش‌فرض | ۱۰۰ میکروثانیه |
| تخصیص منابع تسک | ایستا؛ بدون Heap عمومی |
| IPC | event، mutex، queue، semaphore |
| تسک دوره‌ای | `period_ms` به‌عنوان release gate در زمان‌بند اعمال می‌شود |
| پایش | پشته، CPU، خطا و در صورت فعال‌سازی deadline |
| ایمنی | canary، watchdog، MPU، RAM test، CRC و TCB integrity |

## شروع سریع

</div>

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

<div dir="rtl">

در پروژه STM32CubeMX، HAL timebase را از SysTick جدا کنید و `os_tick()` را داخل `SysTick_Handler` فراخوانی کنید. فایل‌های ZenOS باید با ARM GCC و C++11 کامپایل شوند.

## مستندات

- [راهنمای تعاملی](https://raymon-research-team.github.io/ZenOS-RTOS/)
- [راهنمای API](API_TUTORIAL_FA.md)
- [راهنمای پیکربندی](CONFIG_GUIDE_FA.md)
- [راهنمای ایمنی](SAFETY_MANUAL_FA.md)
- [مقایسه فنی](ZENOS_ADVANTAGES.md)
- [هدر پیکربندی هسته](../ZenOS/ZenOS_Config.hpp)

## ایمنی و استانداردها

ZenOS مکانیسم‌های قابل پیکربندی برای توسعه سیستم‌های حساس ارائه می‌دهد. پروفایل‌های کامپایل‌زمان برای IEC 62304 و IEC 61508 تنظیمات الزامی ZenOS را کنترل می‌کنند، اما به‌تنهایی به معنی گواهی یا انطباق محصول نیستند.

مسئولیت تحلیل ریسک، نیازمندی‌ها، ردیابی، Verification، Validation و فرایند گواهی محصول بر عهده یکپارچه‌ساز است.

## حمایت از پروژه

برای حمایت از توسعه و نگهداری ZenOS می‌توانید از صفحه حمایت پروژه استفاده کنید:

**[Support ZenOS](https://donatr.ee/raymon-research-team/)**

## تماس

**Raymon Research Team — رحمان حیدری**  
[rahman.h22@gmail.com](mailto:rahman.h22@gmail.com)

</div>

<div align="center">

**ZenOS — سادگی · امنیت · سرعت**

MIT License · Raymon Research Team

</div>
