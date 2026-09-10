<div align="center">

<img src="docs/guide-html/ZenOS_logo.svg" alt="ZenOS Logo" width="400" />

**Real-Time Operating System for ARM Cortex-M — written in C++11.**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg?style=for-the-badge)](LICENSE)
[![Version](https://img.shields.io/badge/Version-1.0.1-green.svg?style=for-the-badge)]()
[![Platform](https://img.shields.io/badge/Platform-ARM%20Cortex--M-orange.svg?style=for-the-badge)]()
[![Language](https://img.shields.io/badge/Language-C%2B%2B11-purple.svg?style=for-the-badge)]()

[**🇮🇷 فارسی**](docs/README_FA.md) · [**🇬🇧 English**](README.md)

📘 **[ZenOS Guide](https://raymon-research-team.github.io/ZenOS-RTOS/)**

</div>

---

## About ZenOS

ZenOS is a compact real-time operating system for ARM Cortex-M microcontrollers. It focuses on predictable scheduling, static resource allocation, a small C++11 API, and configurable runtime safety mechanisms.

The kernel provides task management, priority-based scheduling, timing, events, mutexes, bounded queues, semaphores, monitoring, watchdog support, memory protection, RAM/flash integrity checks, and compile-time safety-target configuration.

> **No general-purpose heap in the kernel task model. Static resources. Explicit configuration.**

## Scheduler and priorities

ZenOS uses a priority bitmap with a separate ready queue for each priority level. Priority `0` is reserved for the idle task. Application priorities are numeric: **`1..255`**, where a larger value means a higher priority.

The configuration macro is:

```cpp
#define OS_KERNEL_MAX_PRIORITIES 32
```

The default configuration provides 32 scheduler slots (`0..31`), while the implementation supports configuring up to **256 slots**, giving usable application priorities `1..255`.

For configurations above 32 slots, ZenOS derives the required extension bitmap and queue storage from `OS_KERNEL_MAX_PRIORITIES`; the implementation does not require a fixed 256-entry task table.

## Highlights

| Feature | Current behavior |
|---|---|
| Scheduler | Priority bitmap + per-priority ready queues + eligibility checks |
| Usable priorities | **1..255** when `OS_KERNEL_MAX_PRIORITIES=256` |
| Default priority slots | 32 (`0..31`) |
| Tick | 100 µs by default; configurable from 100–1000 µs |
| Task resources | Static TCB/stack resources; no general-purpose heap |
| IPC | Events, mutexes, bounded FIFO queues, counting semaphores |
| Periodic tasks | `period_ms` is enforced through the scheduler release gate |
| Monitoring | Stack, CPU, error, and optional deadline monitoring |
| Safety | Configurable canary, watchdog, MPU, RAM test, CRC and TCB checks |
| Language | C++11 with C-accessible core declarations |

## Quick start

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

Before starting the scheduler in an STM32CubeMX project:

1. Keep `SysTick` available for ZenOS and move the HAL timebase to a hardware timer such as `TIM1`.
2. Call `os_tick()` from `SysTick_Handler`.
3. Compile ZenOS sources as C++11 with the ARM GCC toolchain.

See the [complete guide](https://raymon-research-team.github.io/ZenOS-RTOS/) for the full setup.

## Architecture

```text
Application Tasks
        │
        ▼
┌───────────────────────────────────────────────┐
│                  ZenOS Kernel                  │
│                                               │
│  Scheduler   IPC             Monitoring       │
│  Bitmap      Events          Stack usage      │
│  Queues      Mutexes         CPU usage        │
│              Queues          Errors           │
│              Semaphores      Deadlines*       │
│                                               │
│  Safety: canary / watchdog / MPU / RAM / CRC  │
└───────────────────────────────────────────────┘
        │
        ▼
STM32 HAL / ARM Cortex-M
```

`*` Deadline monitoring is optional and disabled by default.

## Documentation

| Document | Purpose |
|---|---|
| [Interactive Guide](https://raymon-research-team.github.io/ZenOS-RTOS/) | Bilingual web guide, setup, API, configuration and troubleshooting |
| [API Tutorial](docs/API_TUTORIAL.md) | API reference and practical C++ examples |
| [API Tutorial — فارسی](docs/API_TUTORIAL_FA.md) | راهنمای API و مثال‌های فارسی |
| [Configuration Guide](docs/CONFIG_GUIDE.md) | Every configuration option, default, range and safety profile |
| [Configuration Guide — فارسی](docs/CONFIG_GUIDE_FA.md) | راهنمای کامل پیکربندی |
| [Safety Manual](docs/SAFETY_MANUAL.md) | Safety mechanisms, assumptions and integration limits |
| [Safety Manual — فارسی](docs/SAFETY_MANUAL_FA.md) | راهنمای ایمنی و محدودیت‌های یکپارچه‌سازی |
| [Technical Advantages](docs/ZENOS_ADVANTAGES.md) | Conservative technical comparison and design characteristics |
| [Kernel configuration](ZenOS/ZenOS_Config.hpp) | Source-of-truth configuration header |

## Safety and standards

ZenOS includes mechanisms that can support safety-oriented development, including stack canaries, TCB integrity checks, deadline monitoring, software/hardware watchdog integration, MPU protection, RAM testing, CRC checking and error logging.

The compile-time target profiles for IEC 62304 and IEC 61508 enforce required **ZenOS configuration choices**. They do not certify ZenOS, the firmware, or the final product. System-level risk analysis, verification, validation, traceability and certification remain the responsibility of the product integrator.

## Memory model

ZenOS uses statically sized kernel/task resources. This makes RAM usage explicit and avoids general-purpose heap fragmentation in the RTOS task model. Enabling more than 32 priority slots adds only the derived extension storage required for the additional priority queues and bitmap words.

## Supported targets

ZenOS is designed around ARM Cortex-M and STM32 projects, with the port layer handling supported Cortex-M families and target-specific features such as MPU availability. Always verify hardware-specific capabilities before enabling target-dependent safety features.

## Support the project

ZenOS is developed and maintained by the Raymon Research Team. If the project is useful to you, you can support development, testing and documentation through the project donation page:

**[Support ZenOS](https://donatr.ee/raymon-research-team/)**

## Contact

**Raymon Research Team — Rahman Heidari**  
Email: [rahman.h22@gmail.com](mailto:rahman.h22@gmail.com)

---

<div align="center">

**ZenOS — Simplicity · Security · Speed**

MIT License · Raymon Research Team

</div>
