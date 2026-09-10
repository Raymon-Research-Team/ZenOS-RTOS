<meta name="google-site-verification" content="WDbhN_r-HKmS5ue_mJL8otHIkSioPnhX6Pml04KWFi4" />
<div align="center">

<img src="docs/guide-html/ZenOS_logo.svg" alt="ZenOS Logo" width="400" />

**Real-Time operating system for ARM Cortex-M — written in C++11.**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg?style=for-the-badge)](LICENSE)
[![Version](https://img.shields.io/badge/Version-1.0.1-green.svg?style=for-the-badge)]()
[![Platform](https://img.shields.io/badge/Platform-ARM%20Cortex--M-orange.svg?style=for-the-badge)]()
[![Language](https://img.shields.io/badge/Language-C%2B%2B11-purple.svg?style=for-the-badge)]()
[![Standard](https://img.shields.io/badge/IEC-62304-red.svg?style=for-the-badge)]()
[![Standard](https://img.shields.io/badge/IEC-61508-red.svg?style=for-the-badge)]()

<br>

[**🇮🇷 فارسی**](docs/README_FA.md) | [**🇬🇧 English**](README.md)

📘 **[ZenOS Guide](https://raymon-research-team.github.io/ZenOS-RTOS/)**

<br>

</div>

---

## 🧘 About

The word *Zen* means clarity through simplicity — stripping away the unnecessary until only what matters remains. That's the idea behind this RTOS.

ZenOS handles the hard parts of embedded systems — scheduling, synchronization, memory protection, fault recovery — so you can focus on your actual application. Most things that would normally take dozens of lines of setup code are reduced to a single function call. The kernel figures out the rest.

It's built for ARM Cortex-M microcontrollers and STM32 projects. ZenOS includes configurable safety mechanisms such as stack checking, MPU protection, watchdogs, CRC verification, TCB integrity checks, and optional deadline monitoring. IEC 62304 and IEC 61508 target-profile checks can be enabled at compile time; these checks validate required ZenOS configuration and do not by themselves certify the OS, application, or final product.

> **No heap. No hidden allocations. No surprises.**

### ✨ Highlights

| Feature | Details |
|:--------|:--------|
| 🧩 IPC Primitives | Tasks, mutexes, events, queues, semaphores — all with minimal boilerplate |
| ⬆️ Priority Ceiling | IPC mutex ceiling support for controlling priority inversion |
| 🏥 IEC 62304 | Compile-time target-profile configuration checks |
| 🏭 IEC 61508 | Compile-time target-profile configuration checks |
| 🔒 Static resources | Kernel/task resources are statically sized; no general-purpose heap in the task model |
| ⚡ Fast selection | Priority bitmap + per-priority ready queues with eligibility checks |

### ⚡ Why ZenOS?

| Advantage | ZenOS | FreeRTOS | Zephyr |
|:----------|:------|:---------|:-------|
| **Tick resolution** | **100μs default; configurable 100–1000μs** | Configurable | Configurable |
| **Scheduler** | **Priority bitmap + per-priority ready queues + eligibility checks** | Priority-based scheduler | Priority-based scheduler |
| **Heap allocation** | **No general-purpose heap in the kernel task model** | Configurable | Configurable |
| **Safety mechanisms** | **Configurable canary, MPU, watchdog, RAM, CRC, TCB checks** | Configurable | Configurable |
| **IEC 62304/61508** | **Compile-time target-profile checks** | — | — |
| **Periodic tasks** | **Scheduler release gate** | Application/timer mechanisms | Application/timer mechanisms |
| **IPC ceiling** | **Mutex IPC ceiling support** | Depends on configuration/API | Depends on configuration/API |
| **C++** | **C++11 kernel API** | C/C++ APIs | C/C++ APIs |

> 📖 [Full technical comparison → ZENOS_ADVANTAGES.md](docs/ZENOS_ADVANTAGES.md)

### 🖥️ Supported Families

```
 STM32F1  STM32F2  STM32F4  STM32F7  STM32H7
  M3        M3       M4       M7      Dual M7+M4

 STM32G0  STM32G4  STM32L0  STM32L4  STM32WB
  M0+       M4       M0+       M4       M4
```

---

## 🚀 Quick Start

A complete RTOS application in under 10 lines:

**Two steps before the code:**

> **⚠️ 1. Set HAL timebase to TIM1 in CubeMX:**
> ZenOS uses `SysTick` for its kernel tick. If HAL also uses `SysTick`, they will conflict. In CubeMX, go to **System Core** → **SYS** → **Timebase Source** and set it to `TIM1`. This gives HAL its own 1ms timebase on TIM1 while SysTick stays free for ZenOS.
>
> **⚠️ 2. Add `os_tick()` to `SysTick_Handler`:**
> Open `Src/stm32fxxx_it.c` and call `os_tick()` inside `SysTick_Handler`:
>
> ```c
> #include "ZenOS.hpp"
>
> void SysTick_Handler(void) {
>     os_tick();
> }
> ```
>
> **⚠️ 3. Add the sample task to `Src/main.cpp` as shown below:**

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
    os_task_create(task_blink, 5);
    os_start();
}
```

> 💡 That's it. The kernel takes over from `os_start()` and never returns. Your task sleeps for 500ms between LED toggles, and you don't have to write scheduling code.

### 🏗️ Architecture

```
┌───────────────────────────────────────────────────┐
│                Application Tasks                  │
├───────────────────────────────────────────────────┤
│              ZenOS Kernel (C++11)                 │
│  ┌────────────┬─────────────┬──────────────────┐  │
│  │  Scheduler │     IPC     │  Safety Layer    │  │
│  │  Bitmap    │  Events     │  Stack Canary    │  │
│  │  priority  │  Mutexes    │  MPU Protection  │  │
│  │  queue     │  Queues     │  HW/SW Watchdog  │  │
│  │            │  Semaphores │  RAM Test        │  │
│  │            │             │  CRC Check       │  │
│  │            │             │  Deadline Mon.   │  │
│  └────────────┴─────────────┴──────────────────┘  │
├───────────────────────────────────────────────────┤
│           STM32 HAL (CubeMX-generated)            │
├───────────────────────────────────────────────────┤
│             ARM Cortex-M Hardware                 │
└───────────────────────────────────────────────────┘
```

For the full API guide → [API_TUTORIAL.md](docs/API_TUTORIAL.md) | [راهنمای فارسی](docs/API_TUTORIAL_FA.md)

---

## 📚 Documentation

> 🌐 **[Interactive Guide (HTML)](https://raymon-research-team.github.io/ZenOS-RTOS/)** — Complete bilingual guide with syntax highlighting, configuration wizard, and donate section.

| Document | What's in it |
|:---------|:-------------|
| 🌐 [guide.html](https://raymon-research-team.github.io/ZenOS-RTOS/) | Complete interactive guide (HTML) |
| 📖 [SAFETY_MANUAL.md](docs/SAFETY_MANUAL.md) | Safety architecture, limitations, target-profile details |
| ⚙️ [CONFIG_GUIDE.md](docs/CONFIG_GUIDE.md) | Complete configuration guide — every option, defaults, profiles, target checks |
| ⚙️ [CONFIG_GUIDE_FA.md](docs/CONFIG_GUIDE_FA.md) | راهنمای کامل پیکربندی (فارسی) |
| 📘 [API_TUTORIAL.md](docs/API_TUTORIAL.md) | Complete API guide with examples (English) |
| 📗 [API_TUTORIAL_FA.md](docs/API_TUTORIAL_FA.md) | Complete API guide with examples (Persian) |
| 🤝 [CONTRIBUTING.md](CONTRIBUTING.md) | How to contribute |
| ⚡ [ZENOS_ADVANTAGES.md](docs/ZENOS_ADVANTAGES.md) | Technical comparison and design characteristics |
| ⚙️ [ZenOS_Config.hpp](ZenOS/ZenOS_Config.hpp) | Source-of-truth configuration header |

---

## 🛡️ Safety Features

Safety and monitoring mechanisms are **configurable** in `ZenOS_Config.hpp`; they are not all enabled by default.

| Mechanism | Config Macro | What it does |
|:----------|:-------------|:-------------|
| 🔍 Stack Canary | `OS_SAFETY_STACK_CHECK` | Stack canary / bounds diagnostics when enabled |
| 🔐 TCB Integrity | `OS_MONITOR_TCB_INTEGRITY` | Detects TCB corruption when enabled |
| ⏱️ Deadline Monitor | `OS_MONITOR_DEADLINE` | Detects deadline misses when enabled |
| 📋 Error Log | `OS_MONITOR_ERROR_LOG` | Records kernel error diagnostics |
| 🐕 HW Watchdog | `OS_SAFETY_HW_WATCHDOG` | Hardware watchdog integration when enabled |
| 💤 SW Watchdog | `OS_SAFETY_SOFT_WATCHDOG` | Software watchdog monitoring when enabled |
| 🧪 RAM Test | `OS_SAFETY_RAM_TEST` | RAM integrity testing when enabled |
| ✅ CRC Check | `OS_SAFETY_CRC_CHECK` | Flash/integrity checking when enabled |
| 🛡️ MPU | `OS_SAFETY_MPU` | Memory protection when supported and enabled |

---

## 💰 Support ZenOS

This is a solo project. I build it, test it, write the docs, and maintain it — all in my spare time, without funding.

> **📝 Note:** ZenOS provides compile-time target profiles for IEC 62304 (medical) and IEC 61508 (industrial) configuration requirements. These profiles and safety mechanisms do not constitute official certification; formal product-level verification, validation, traceability, audits, and certification remain the responsibility of the project/product owner.

If you find ZenOS useful, whether for learning, prototyping, or shipping a product, consider supporting its continued development.

### 🎯 Where the money goes

<table>
<tr>
<td align="center" width="25%">

### 🏥 Certification
Formal safety documentation, traceability, audits, verification and certification activities require significant resources.

</td>
<td align="center" width="25%">

### 🔧 Hardware
New STM32 boards, logic analyzers, JTAG probes — every supported family needs its own test setup.

</td>
<td align="center" width="25%">

### ☁️ Infrastructure
CI pipelines, cloud builds, automated test runs. Keeping everything running takes resources.

</td>
<td align="center" width="25%">

### 📖 Docs & Community
Writing good documentation takes time. Answering issues takes time. Both matter.

</td>
</tr>
</table>

## 💸 Donate

Your donation directly funds development, testing, certification, and documentation.

> 💡 **Click a wallet address to copy it into your wallet app. Click the QR code to open it in a new tab.**

<table>
<tr>
<td align="center" width="50%">

🟠 **Bitcoin (BTC)**

[`bc1qd39vgmnweuzh5hp2cqm4cnh782xga6wph3v650`](bitcoin:bc1qd39vgmnweuzh5hp2cqm4cnh782xga6wph3v650)

</td>
<td rowspan="2" align="center" width="50%">

<a href="https://donatr.ee/raymon-research-team/" target="_blank" rel="noopener" title="Open donation page in a new tab"><img src="docs/guide-html/donate-qr.png" alt="Donate QR Code" width="180" /></a>

</td>
</tr>
<tr>
<td align="center" width="50%">

🔵 **Ethereum (ETH) / USDT (ERC-20)**

[`0x1C21c39324F65a38Fb8de9ccB92aB01FdeD1534C`](ethereum:0x1C21c39324F65a38Fb8de9ccB92aB01FdeD1534C)

</td>
</tr>
</table>

> ☕ Even a few dollars helps. If everyone who cloned this repo bought me a coffee, I could afford a proper test bench for every STM32 family and hire someone to help with certification paperwork.

> 📩 **After making a donation, please send an email to [rahman.h22@gmail.com](mailto:rahman.h22@gmail.com) with your transaction ID or wallet address so I can personally thank you.**

### 🌟 Other ways to help

<td align="center" width="100%">
<table>
<tr>
<td align="center" width="25%" >⭐<br><b>Star</b><br>the repo</td>
<td align="center" width="25%" >🐛<br><b>Report</b><br>a bug</td>
<td align="center" width="25%" >📝<br><b>Write</b><br>a tutorial</td>
<td align="center" width="25%" >🗣️<br><b>Tell</b><br>someone</td>
</tr>
</table>
</td>

---

## 📬 Contact

**Raymon Research Team** — Rahman Heidari

📧 Email: [rahman.h22@gmail.com](mailto:rahman.h22@gmail.com)

For questions, suggestions, or collaboration opportunities, feel free to reach out. I'm always happy to hear from developers using ZenOS in their projects.

---

<div align="center">

<br>

**Built with ❤️ for the embedded community**

[![GitHub stars](https://img.shields.io/github/stars/Raymon-Research-Team/ZenOS-RTOS?style=social)]()

<br>

*ZenOS — (Simplicity , Security , Speed)*

</div>
