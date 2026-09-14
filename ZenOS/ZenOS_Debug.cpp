/**
 * @file    ZenOS_Debug.cpp
 * @brief   ZenOS RTOS — Debug UART Subsystem Implementation
 *
 * Provides register-level UART init and lightweight formatted output
 * for live runtime tracing.  No HAL, no CMSIS, no heap allocation.
 *
 * @author  Rahman Heidari <rahman.h22@gmail.com> — Raymon Research Team
 * @version 1.1.0
 */

#include "ZenOS_Internal.hpp"
#include "ZenOS_Debug.hpp"

#if OS_DEBUG_UART

extern "C" uint32_t SystemCoreClock;

/* ═══════════════ SEGGER RTT Backend ═══════════════ */
#if (OS_DEBUG_BACKEND == OS_DEBUG_BACKEND_RTT)

/* RTT Control Block (SEGGER RTT compatible layout)
 * Placed at a fixed SRAM address so the debug probe can find it.
 * Ref: SEGGER RTT specification — minimal compatible implementation. */
typedef struct {
    char     acID[16];        /* "SEGGER RTT" + padding */
    uint32_t MaxNumUpBuffers;
    uint32_t MaxNumDownBuffers;
} OS_RTT_CB_Header;

typedef struct {
    const char* sName;
    char*       pBuffer;
    uint32_t    SizeOfBuffer;
    uint32_t    WrOff;
    volatile uint32_t RdOff;
    uint32_t    Flags;
} OS_RTT_UpBuffer;

static char os_rtt_buffer[OS_RTT_BUFFER_SIZE];
static OS_RTT_CB_Header os_rtt_cb_header OS_ALIGNED(16) = {
    "SEGGER RTT\0\0\0\0\0\0", 1, 0
};
static OS_RTT_UpBuffer os_rtt_up_buffer = {
    "Terminal", nullptr, 0, 0, 0, 0
};

static volatile bool os_rtt_initialized = false;

static void os_rtt_init(void) {
    os_rtt_up_buffer.pBuffer = os_rtt_buffer;
    os_rtt_up_buffer.SizeOfBuffer = OS_RTT_BUFFER_SIZE;
    os_rtt_up_buffer.WrOff = 0;
    os_rtt_up_buffer.RdOff = 0;
    os_rtt_up_buffer.Flags = 0; /* No skip mode */
    /* The control block header is placed at OS_RTT_BUFFER_ADDR so the
       debug probe can find it via JTAG/SWD memory scan. The actual
       buffer follows immediately after. */
    os_rtt_initialized = true;
}

static void os_rtt_putc(char c) {
    if (!os_rtt_initialized) os_rtt_init();
    uint32_t next = (os_rtt_up_buffer.WrOff + 1) % OS_RTT_BUFFER_SIZE;
    if (next == os_rtt_up_buffer.RdOff) return; /* Buffer full — drop byte */
    os_rtt_buffer[os_rtt_up_buffer.WrOff] = c;
    os_rtt_up_buffer.WrOff = next;
}

void os_debug_putc(char c) {
    os_rtt_putc(c);
}

void os_debug_puts(const char* str) {
    if (!str) return;
    while (*str) {
        if (*str == '\n') os_rtt_putc('\r');
        os_rtt_putc(*str++);
    }
}

void os_debug_init(void) {
    os_rtt_init();
}

#else /* UART backend */

/* ═══════════════ USART SR Bit Masks ═══════════════ */
#define OS_USART_SR_TXE   (1UL << 7)   /* Transmit data register empty */
#define OS_USART_SR_TC    (1UL << 6)   /* Transmission complete */
#define OS_USART_SR_RXNE  (1UL << 5)   /* Read data register not empty */
#define OS_USART_CR1_UE   (1UL << 13)  /* USART enable */
#define OS_USART_CR1_TE   (1UL << 3)   /* Transmitter enable */
#define OS_USART_CR1_RE   (1UL << 2)   /* Receiver enable */

/* ═══════════════ Core Output ═══════════════ */

void os_debug_init(void) {
    /* In CubeMX coexistence mode, HAL (MX_USART1_UART_Init) already
       configures the USART peripheral, GPIO pins, and clocks.
       We only need to verify the USART is enabled — no re-init needed.
       If running standalone (no HAL), the user must call os_debug_init()
       BEFORE os_init(), or configure USART separately. */
    volatile uint32_t* usart_cr1 = (volatile uint32_t*)(OS_DEBUG_USART_DEFAULT_BASE + 0x0CUL);
    if (!(*usart_cr1 & OS_USART_CR1_UE)) {
        /* USART not enabled — HAL didn't init it. Do minimal bring-up. */
        volatile uint32_t* rcc_enr = (volatile uint32_t*)OS_DEBUG_RCC_ENR;
        *rcc_enr |= (1UL << OS_DEBUG_RCC_GPIO_BIT);
        *rcc_enr |= (1UL << OS_DEBUG_RCC_USART_BIT);

#if defined(STM32F1xx)
        volatile uint32_t* cr = (volatile uint32_t*)(OS_DEBUG_GPIO_DEFAULT_BASE + OS_DEBUG_TX_CRH_OFFSET);
        uint32_t pin = OS_DEBUG_TX_PIN;
        uint32_t shift = (pin & 7) * 4;
        *cr = (*cr & ~(0xFUL << shift)) | (0xBUL << shift);
#else
        volatile uint32_t* moder = (volatile uint32_t*)(OS_DEBUG_GPIO_DEFAULT_BASE + OS_GPIO_MODER_OFFSET);
        volatile uint32_t* afrh  = (volatile uint32_t*)(OS_DEBUG_GPIO_DEFAULT_BASE + OS_GPIO_AFRH_OFFSET);
        uint32_t pin = OS_DEBUG_TX_PIN;
        *moder = (*moder & ~(3UL << (pin * 2))) | (2UL << (pin * 2));
        if (pin >= 8) *afrh = (*afrh & ~(0xFUL << ((pin - 8) * 4))) | (7UL << ((pin - 8) * 4));
        else {
            volatile uint32_t* afrl = (volatile uint32_t*)(OS_DEBUG_GPIO_DEFAULT_BASE + OS_GPIO_AFRL_OFFSET);
            *afrl = (*afrl & ~(0xFUL << (pin * 4))) | (7UL << (pin * 4));
        }
#endif
        volatile uint32_t* usart_brr = (volatile uint32_t*)(OS_DEBUG_USART_DEFAULT_BASE + 0x08UL);
        *usart_cr1 = 0;
        *usart_brr = SystemCoreClock / OS_DEBUG_BAUDRATE;
        *usart_cr1 = (1UL << 13) | (1UL << 3); /* UE | TE */
    }
    /* USART is already enabled by HAL — nothing more to do */
}

void os_debug_putc(char c) {
    /* Polling: wait for TXE, then write.  Timeout after ~1ms to avoid
       hanging if UART is stuck (e.g., pin reconfigured by CubeMX). */
    uint32_t timeout = (SystemCoreClock / 1000) | 1;
    while (!(OS_DEBUG_USART_SR & OS_USART_SR_TXE) && --timeout) {}
    if (timeout) OS_DEBUG_USART_DR = (uint32_t)c;
}

void os_debug_puts(const char* str) {
    if (!str) return;
    while (*str) {
        if (*str == '\n') os_debug_putc('\r');
        os_debug_putc(*str++);
    }
}

/* Ring buffer mode (UART only) — moved after os_debug_init */
#if OS_DEBUG_RING_BUFFER
static volatile char     os_dbg_ring[OS_DEBUG_RING_SIZE];
static volatile uint32_t os_dbg_ring_head = 0;
static volatile uint32_t os_dbg_ring_tail = 0;
static volatile uint32_t os_dbg_ring_count = 0;

static void os_dbg_ring_put(char c) {
    uint32_t cs = os_critical_enter();
    if (os_dbg_ring_count < OS_DEBUG_RING_SIZE) {
        os_dbg_ring[os_dbg_ring_head] = c;
        os_dbg_ring_head = (os_dbg_ring_head + 1) % OS_DEBUG_RING_SIZE;
        os_dbg_ring_count++;
    }
    os_critical_exit(cs);
}

static void os_dbg_flush(void) {
    /* Timeout guard: if UART TX is stuck, don't block the kernel */
    uint32_t timeout = (SystemCoreClock / 1000) | 1;
    while (os_dbg_ring_count > 0 && timeout > 0) {
        while (!(OS_DEBUG_USART_SR & OS_USART_SR_TXE) && --timeout) {}
        if (timeout == 0) break;
        uint32_t cs = os_critical_enter();
        OS_DEBUG_USART_DR = os_dbg_ring[os_dbg_ring_tail];
        os_dbg_ring_tail = (os_dbg_ring_tail + 1) % OS_DEBUG_RING_SIZE;
        os_dbg_ring_count--;
        os_critical_exit(cs);
        timeout = (SystemCoreClock / 1000) | 1;
    }
    timeout = (SystemCoreClock / 1000) | 1;
    while (!(OS_DEBUG_USART_SR & OS_USART_SR_TC) && --timeout) {}
}
#endif /* OS_DEBUG_RING_BUFFER */

/* ═══════════════ Lightweight itoa ═══════════════ */

static void os_dbg_write_uint(uint32_t val) {
    char buf[11];
    int i = 10;
    buf[i] = '\0';
    if (val == 0) { buf[--i] = '0'; }
    else { while (val > 0) { buf[--i] = '0' + (val % 10); val /= 10; } }
    os_debug_puts(&buf[i]);
}

static void os_dbg_write_hex(uint32_t val, bool upper) {
    char buf[9];
    buf[8] = '\0';
    for (int i = 0; i < 8; i++) {
        uint32_t nib = (val >> (28 - i * 4)) & 0xF;
        if (nib < 10) buf[i] = '0' + nib;
        else buf[i] = (upper ? 'A' : 'a') + nib - 10;
    }
    os_debug_puts(buf);
}

static void os_dbg_write_int(int32_t val) {
    if (val < 0) { os_debug_putc('-'); val = -val; }
    os_dbg_write_uint((uint32_t)val);
}

/* ═══════════════ Printf-Like Formatter ═══════════════ */

/* Simple stack-only formatter — supports %d, %u, %x, %X, %s, %c, %%
 * Field width with zero-padding: %08x, %4d, etc.
 * Maximum format string: 128 bytes.
 * No heap allocation, no varargs overhead beyond the call site. */

/* We use a minimal va_list implementation for ARM to avoid stdarg.h
   dependency when building with -ffreestanding.  On ARM, va_list is
   a pointer and va_arg increments it.  For varargs, the caller passes
   arguments on the stack (AAPCS) and we step through them. */
typedef char* os_va_list;
#define os_va_start(ap, last) ((ap) = (char*)&(last) + sizeof(last))
#define os_va_arg(ap, type)   (*(type*)(((ap) += sizeof(type)) - sizeof(type)))
#define os_va_end(ap)         ((void)0)

void os_debug_printf(const char* fmt, ...) {
    if (!fmt) return;
    os_va_list ap;
    os_va_start(ap, fmt);
    uint32_t fmt_remaining = 128; /* Max format string walk length (documented limit) */

    for (const char* p = fmt; *p && fmt_remaining > 0; p++, fmt_remaining--) {
        if (*p != '%') {
            if (*p == '\n') os_debug_putc('\r');
            os_debug_putc(*p);
            continue;
        }

        p++;  /* skip '%' */

        /* Parse field width and zero-padding */
        bool pad_zero = false;
        uint32_t width = 0;
        if (*p == '0') { pad_zero = true; p++; fmt_remaining--; }
        while (*p >= '0' && *p <= '9' && fmt_remaining > 0) {
            width = width * 10 + (*p - '0');
            p++; fmt_remaining--;
        }
        if (width > 64) width = 64; /* Cap width to prevent local DoS */

        switch (*p) {
        case 'd': {
            int32_t val = os_va_arg(ap, int32_t);
            /* Padding */
            if (pad_zero && val < 0) { os_debug_putc('-'); val = -val; }
            char buf[12];
            int i = 11;
            buf[i] = '\0';
            if (val == 0) { buf[--i] = '0'; }
            else {
                uint32_t uval = (uint32_t)val;
                while (uval > 0) { buf[--i] = '0' + (uval % 10); uval /= 10; }
            }
            if (val < 0 && !pad_zero) buf[--i] = '-';
            uint32_t len = 11 - i;
            while (len < width) { os_debug_putc(pad_zero ? '0' : ' '); len++; }
            if (val < 0 && pad_zero) os_debug_putc('-');
            os_debug_puts(&buf[i]);
            break;
        }
        case 'u': {
            uint32_t val = os_va_arg(ap, uint32_t);
            char buf[11];
            int i = 10;
            buf[i] = '\0';
            if (val == 0) { buf[--i] = '0'; }
            else { while (val > 0) { buf[--i] = '0' + (val % 10); val /= 10; } }
            uint32_t len = 10 - i;
            while (len < width) { os_debug_putc(pad_zero ? '0' : ' '); len++; }
            os_debug_puts(&buf[i]);
            break;
        }
        case 'x': {
            uint32_t val = os_va_arg(ap, uint32_t);
            char buf[9];
            buf[8] = '\0';
            for (int j = 0; j < 8; j++) {
                uint32_t nib = (val >> (28 - j * 4)) & 0xF;
                buf[j] = (nib < 10) ? ('0' + nib) : ('a' + nib - 10);
            }
            /* Skip leading zeros but keep at least one digit */
            int start = 0;
            while (start < 7 && buf[start] == '0') start++;
            uint32_t len = 8 - start;
            while (len < width) { os_debug_putc(pad_zero ? '0' : ' '); len++; }
            os_debug_puts(&buf[start]);
            break;
        }
        case 'X': {
            uint32_t val = os_va_arg(ap, uint32_t);
            char buf[9];
            buf[8] = '\0';
            for (int j = 0; j < 8; j++) {
                uint32_t nib = (val >> (28 - j * 4)) & 0xF;
                buf[j] = (nib < 10) ? ('0' + nib) : ('A' + nib - 10);
            }
            int start = 0;
            while (start < 7 && buf[start] == '0') start++;
            uint32_t len = 8 - start;
            while (len < width) { os_debug_putc(pad_zero ? '0' : ' '); len++; }
            os_debug_puts(&buf[start]);
            break;
        }
        case 's': {
            const char* s = os_va_arg(ap, const char*);
            if (!s) s = "(null)";
            uint32_t len = 0;
            while (s[len]) len++;
            while (len < width) { os_debug_putc(' '); len++; }
            os_debug_puts(s);
            break;
        }
        case 'c': {
            char c = (char)os_va_arg(ap, int);
            os_debug_putc(c);
            break;
        }
        case '%':
            os_debug_putc('%');
            break;
        default:
            os_debug_putc('%');
            os_debug_putc(*p);
            break;
        }
    }

    os_va_end(ap);

#if OS_DEBUG_RING_BUFFER
    os_dbg_flush();
#endif
}

/* ═══════════════ Log with Level Prefix ═══════════════ */

static const char* os_dbg_level_str(uint8_t level) {
    switch (level) {
        case 0: return "[E]";
        case 1: return "[W]";
        case 2: return "[I]";
        case 3: return "[T]";
        default: return "[?]";
    }
}

void os_debug_log(uint8_t level, const char* fmt, ...) {
    if (level > OS_DEBUG_MAX_LEVEL) return;

    os_debug_puts(os_dbg_level_str(level));
    os_debug_putc(' ');
    os_dbg_write_uint(tick_count);
    os_debug_putc(' ');

    /* Print formatted message using the same lightweight formatter */
    if (!fmt) { os_debug_puts("(null)\r\n"); return; }

    os_va_list ap;
    os_va_start(ap, fmt);
    uint32_t fmt_remaining = 128; /* Max format string walk length */

    for (const char* p = fmt; *p && fmt_remaining > 0; p++, fmt_remaining--) {
        if (*p != '%') {
            if (*p == '\n') os_debug_putc('\r');
            os_debug_putc(*p);
            continue;
        }
        p++;
        /* Parse field width */
        bool pad_zero = false;
        uint32_t width = 0;
        if (*p == '0') { pad_zero = true; p++; fmt_remaining--; }
        while (*p >= '0' && *p <= '9' && fmt_remaining > 0) { width = width * 10 + (*p - '0'); p++; fmt_remaining--; }
        if (width > 64) width = 64; /* Cap width to prevent local DoS */
        switch (*p) {
            case 'u': { uint32_t v = os_va_arg(ap, uint32_t); os_dbg_write_uint(v); break; }
            case 'd': { int32_t v = os_va_arg(ap, int32_t); os_dbg_write_int(v); break; }
            case 'x': { uint32_t v = os_va_arg(ap, uint32_t); os_dbg_write_hex(v, false); break; }
            case 'X': { uint32_t v = os_va_arg(ap, uint32_t); os_dbg_write_hex(v, true); break; }
            case 's': { const char* s = os_va_arg(ap, const char*); os_debug_puts(s ? s : "(null)"); break; }
            case 'c': { os_debug_putc((char)os_va_arg(ap, int)); break; }
            default:  os_debug_putc(*p); break;
        }
    }

    os_va_end(ap);
    os_debug_puts("\r\n");
}

/* ═══════════════ Boot Banner ═══════════════ */

void os_debug_boot_banner(void) {
    os_debug_puts("\r\n");
    os_debug_puts("=========================================\r\n");
    os_debug_puts("  ZenOS RTOS v");
    os_debug_puts(OS_VERSION_STRING);
    os_debug_puts("\r\n");
    os_debug_puts("  Family:  ");
    os_debug_puts(OS_FAMILY_NAME);
    os_debug_puts("\r\n");
    os_debug_puts("  Arch:    ");
    os_debug_puts(OS_ARCH_NAME);
    os_debug_puts("\r\n");
    os_debug_puts("  Clock:   ");
    os_dbg_write_uint(SystemCoreClock);
    os_debug_puts(" Hz\r\n");
    os_debug_puts("  Tick:    ");
    os_dbg_write_uint(OS_KERNEL_TICK_PERIOD_US);
    os_debug_puts(" us\r\n");
    os_debug_puts("  Build:   ");
#if defined(OS_OPT_LEVEL_STRING)
    os_debug_puts(OS_OPT_LEVEL_STRING);
#else
    os_debug_puts("O+");
#endif
    os_debug_puts("\r\n");
    os_debug_puts("=========================================\r\n");
}

/* ═══════════════ Context Switch Trace ═══════════════ */

void os_debug_context_switch(const char* from_name, uint8_t from_id,
                              const char* to_name, uint8_t to_id) {
    os_debug_puts("CSW ");
    os_dbg_write_uint(tick_count);
    os_debug_puts(" [");
    os_debug_puts(from_name ? from_name : "?");
    os_debug_putc(':');
    os_dbg_write_uint(from_id);
    os_debug_puts("]->[");
    os_debug_puts(to_name ? to_name : "?");
    os_debug_putc(':');
    os_dbg_write_uint(to_id);
    os_debug_puts("]\r\n");
}

/* ═══════════════ Fault Dump ═══════════════ */

/* os_last_fault declared in ZenOS_Internal.hpp */

void os_debug_fault_dump(void) {
    os_debug_puts("\r\n===== HARDFAULT DUMP =====\r\n");
    os_debug_puts("CFSR:  0x"); os_dbg_write_hex(os_last_fault.cfsr, true); os_debug_puts("\r\n");
    os_debug_puts("HFSR:  0x"); os_dbg_write_hex(os_last_fault.hfsr, true); os_debug_puts("\r\n");
    os_debug_puts("MMFAR: 0x"); os_dbg_write_hex(os_last_fault.mmfar, true); os_debug_puts("\r\n");
    os_debug_puts("BFAR:  0x"); os_dbg_write_hex(os_last_fault.bfar, true); os_debug_puts("\r\n");
    os_debug_puts("R0:    0x"); os_dbg_write_hex(os_last_fault.r0, true); os_debug_puts("\r\n");
    os_debug_puts("R1:    0x"); os_dbg_write_hex(os_last_fault.r1, true); os_debug_puts("\r\n");
    os_debug_puts("R2:    0x"); os_dbg_write_hex(os_last_fault.r2, true); os_debug_puts("\r\n");
    os_debug_puts("R3:    0x"); os_dbg_write_hex(os_last_fault.r3, true); os_debug_puts("\r\n");
    os_debug_puts("R12:   0x"); os_dbg_write_hex(os_last_fault.r12, true); os_debug_puts("\r\n");
    os_debug_puts("LR:    0x"); os_dbg_write_hex(os_last_fault.lr, true); os_debug_puts("\r\n");
    os_debug_puts("PC:    0x"); os_dbg_write_hex(os_last_fault.pc, true); os_debug_puts("\r\n");
    os_debug_puts("xPSR:  0x"); os_dbg_write_hex(os_last_fault.xpsr, true); os_debug_puts("\r\n");
    os_debug_puts("Task:  "); os_debug_puts(os_last_fault.task_name ? os_last_fault.task_name : "?");
    os_debug_puts(" (id="); os_dbg_write_uint(os_last_fault.task_id); os_debug_puts(")\r\n");
    os_debug_puts("SP:    0x"); os_dbg_write_hex((uint32_t)os_last_fault.task_stack_top, true);
    os_debug_puts(" (size="); os_dbg_write_uint(os_last_fault.task_stack_size * 4); os_debug_puts(")\r\n");
    os_debug_puts("===== END DUMP =====\r\n");
}

/* ═══════════════ Periodic Statistics ═══════════════ */

#if OS_DEBUG_STATS
void os_debug_stats(void) {
    os_debug_puts("\r\n--- Stats @ tick ");
    os_dbg_write_uint(tick_count);
    os_debug_puts(" ---\r\n");

#if OS_MONITOR_ENABLED
    os_debug_puts("  CPU total: ");
    os_dbg_write_uint(os_get_cpu_usage_total());
    os_debug_puts("%\r\n");

    /* Per-task stack watermark */
    uint8_t rep_cnt = os_get_stack_report_count();
    for (uint8_t i = 0; i < rep_cnt; i++) {
        os_stack_report_entry_t rep;
        if (!os_get_stack_report(i, &rep)) continue;
        os_debug_puts("  [");
        os_dbg_write_uint(rep.id);
        os_debug_puts("] ");
        os_debug_puts(rep.name ? rep.name : "?");
        os_debug_puts(": peak=");
        os_dbg_write_uint(rep.peak_bytes);
        os_debug_puts("/");
        os_dbg_write_uint(rep.size_bytes);
        os_debug_puts(" (");
        uint32_t pct = (rep.size_bytes > 0) ? (rep.peak_bytes * 100 / rep.size_bytes) : 0;
        os_dbg_write_uint(pct);
        os_debug_puts("%)\r\n");
    }
#endif

    os_debug_puts("--- end stats ---\r\n");
}
#endif /* OS_DEBUG_STATS */

#endif /* OS_DEBUG_BACKEND == UART */

#else /* OS_DEBUG_UART == 0 — no-op stubs */
/* Empty — all inline stubs in ZenOS_Debug.hpp handle the zero-overhead case */
#endif /* OS_DEBUG_UART */
