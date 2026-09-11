/**
 * ZenOS portable safety layer.
 * Hardware-specific watchdog/CRC hooks are BSP-owned.
 */
#define OS_BUILD
#define os_stack_check_all os_stack_check_all_legacy
#define OS_Fault_C_Handler OS_Fault_C_Handler_legacy
#define OS_Fault_Handler OS_Fault_Handler_legacy
#include "ZenOS_Internal.hpp"
#undef os_stack_check_all
#undef OS_Fault_C_Handler
#undef OS_Fault_Handler

extern "C" uint32_t SystemCoreClock;

uint32_t os_get_wdg_reset_count(void)      { return wdg_reset_count; }
uint32_t os_get_stack_recovery_count(void) { return stack_recovery_count; }
uint32_t os_get_error_count(void)          { return error_total; }
uint32_t os_get_expected_error_count(void) { return error_expected; }
uint32_t os_get_unexpected_error_count(void) {
    return (error_total >= error_expected) ? (error_total - error_expected) : 0U;
}
OSError os_get_last_error(void) { return error_last; }
uint32_t os_in_safe(void) { return os_safe_depth; }

extern "C" void os_error_expect_begin(void) {
    uint32_t cs = os_critical_enter();
    ++error_expect_depth;
    os_critical_exit(cs);
}
extern "C" void os_error_expect_end(void) {
    uint32_t cs = os_critical_enter();
    if (error_expect_depth) --error_expect_depth;
    os_critical_exit(cs);
}

void os_report_error(OSError code) {
    uint32_t cs = os_critical_enter();
    ++error_total;
    if (error_expect_depth) ++error_expected;
    error_last = code;
    os_critical_exit(cs);
}

void os_stack_check_all(void) {
    uint32_t cs = os_critical_enter();
    for (TCB* task = task_list; task; task = task->next) {
        if (task == &idle_tcb || task->state == TaskState::INACTIVE || !task->stack_base)
            continue;
        const uintptr_t lo = (uintptr_t)task->stack_base;
        const uintptr_t hi = lo + (uintptr_t)task->stack_size * sizeof(uint32_t);
        const uintptr_t sp = (uintptr_t)task->stack_top;
        const uintptr_t guard = lo + (uintptr_t)OS_STACK_CANARY_COUNT * sizeof(uint32_t);
        bool bad = (sp < guard || sp > hi);
        if (!bad) {
            for (uint32_t i = 0; i < OS_STACK_CANARY_COUNT; ++i) {
                if (task->stack_base[i] != OS_STACK_CANARY) { bad = true; break; }
            }
        }
        if (!bad) continue;
        os_report_error(OSError::STACK_OVERFLOW);
        os_pq_remove(task);
#if OS_MONITOR_TCB_INTEGRITY
        if (!os_tcb_check_magic(task)) { task->state = TaskState::INACTIVE; continue; }
#endif
        if (task == current_task) {
            task->state = TaskState::INACTIVE;
            current_task = nullptr;
            OS_SCB_ICSR = OS_ICSR_PENDSVSET_Msk;
            continue;
        }
        if (stack_recovery_count < OS_SAFETY_TASK_MAX_RECOVERY) {
            ++stack_recovery_count;
            os_reset_task_internal(task);
        } else {
            task->state = TaskState::INACTIVE;
        }
    }
    os_critical_exit(cs);
}

/* Never return from a fault through a possibly-corrupted PSP. */
extern "C" void OS_Fault_C_Handler(void) {
    os_report_error(OSError::HARDFAULT);
    os_safe_depth = 0;
    uint32_t cs = os_critical_enter();
    if (current_task && current_task != &idle_tcb)
        current_task->state = TaskState::INACTIVE;
    current_task = nullptr;
    os_critical_exit(cs);
    __asm volatile("dsb" ::: "memory");
    for (;;) __asm volatile("wfi" ::: "memory");
}

extern "C" OS_NAKED OS_USED void OS_Fault_Handler(void) {
    __asm volatile(
        "cpsid i                 \n"
        "bl OS_Fault_C_Handler   \n"
        "1: wfi                  \n"
        "b 1b                    \n"
    );
}

#if OS_MONITOR_ERROR_LOG
static OSErrorEntry error_log[OS_MONITOR_ERROR_LOG_SIZE] OS_ALIGNED(4);
static uint32_t error_log_head = 0;
static uint32_t error_log_count = 0;
static uint32_t error_log_total = 0;
extern "C" void os_log_error(OSError code, uint8_t severity) {
    uint32_t cs = os_critical_enter();
    OSErrorEntry* e = &error_log[error_log_head];
    e->timestamp_tick = tick_count;
    e->code = code;
    e->task_id = current_task ? current_task->id : 0xFF;
    e->severity = (ErrorSeverity)severity;
    error_log_head = (error_log_head + 1U) % OS_MONITOR_ERROR_LOG_SIZE;
    if (error_log_count < OS_MONITOR_ERROR_LOG_SIZE) ++error_log_count;
    ++error_log_total;
    os_critical_exit(cs);
}
extern "C" OSErrorEntry os_get_error_log_entry(uint32_t index) {
    OSErrorEntry empty = {};
    uint32_t cs = os_critical_enter();
    if (index >= error_log_count) { os_critical_exit(cs); return empty; }
    uint32_t pos = (error_log_head + OS_MONITOR_ERROR_LOG_SIZE - 1U - index) % OS_MONITOR_ERROR_LOG_SIZE;
    OSErrorEntry result = error_log[pos];
    os_critical_exit(cs);
    return result;
}
extern "C" uint32_t os_get_error_log_count(void) { return error_log_count; }
extern "C" uint32_t os_get_error_log_total(void) { return error_log_total; }
#endif

#if OS_SAFETY_RAM_TEST
static uint32_t* ram_test_current = nullptr;
static uint32_t ram_test_errors = 0;
static bool ram_test_complete_flag = false;
extern "C" void os_ram_test_step(void) {
    if (ram_test_complete_flag) return;
    if (!ram_test_current) ram_test_current = (uint32_t*)OS_RAM_TEST_START;
    if (ram_test_current >= (uint32_t*)OS_RAM_TEST_END) { ram_test_complete_flag = true; return; }
    uint32_t old = *ram_test_current;
    *ram_test_current = ~old;
    if (*ram_test_current != ~old) { ++ram_test_errors; os_report_error(OSError::RAM_TEST_FAIL); }
    *ram_test_current = old;
    ++ram_test_current;
}
extern "C" uint8_t os_ram_test_progress(void) {
    const uint32_t total = OS_RAM_TEST_END - OS_RAM_TEST_START;
    if (!total) return 100;
    uintptr_t done = ram_test_current ? ((uintptr_t)ram_test_current - OS_RAM_TEST_START) : 0U;
    if (done > total) done = total;
    return (uint8_t)((done * 100UL) / total);
}
extern "C" uint32_t os_get_ram_test_error_count(void) { return ram_test_errors; }
extern "C" bool os_ram_test_complete(void) { return ram_test_complete_flag; }
#endif

/* BSP hooks: no hard-coded STM32 register addresses in the kernel safety layer. */
extern "C" OS_USED __attribute__((weak)) void os_platform_watchdog_feed(void) {}
extern "C" OS_USED __attribute__((weak)) uint32_t os_platform_watchdog_reset_flags(void) { return 0U; }
#if OS_SAFETY_HW_WATCHDOG
static volatile uint32_t hw_wdg_reset_count_var = 0;
extern "C" void os_hw_watchdog_feed(void) { os_platform_watchdog_feed(); }
extern "C" void os_hw_watchdog_check(void) {
    if (current_task && os_get_error_count() < 10U) os_platform_watchdog_feed();
}
extern "C" uint32_t os_get_hw_wdg_reset_count(void) {
    if (os_platform_watchdog_reset_flags()) ++hw_wdg_reset_count_var;
    return hw_wdg_reset_count_var;
}
#endif

#if OS_SAFETY_CRC_CHECK
static uint32_t crc_error_count = 0;
static bool crc_complete = false;
static uint32_t crc_expected = 0;
extern "C" uint32_t os_platform_crc32(const void*, uint32_t) __attribute__((weak));
extern "C" uint32_t os_platform_crc32(const void*, uint32_t) { return 0U; }
extern "C" void os_crc_init(void) {
    crc_expected = os_platform_crc32((const void*)OS_FLASH_START, OS_FLASH_SIZE);
    crc_complete = false;
}
extern "C" void os_crc_check_step(void) {
    if (crc_complete) return;
    uint32_t now = os_platform_crc32((const void*)OS_FLASH_START, OS_FLASH_SIZE);
    crc_complete = true;
    if (now != crc_expected) { ++crc_error_count; os_report_error(OSError::HARDFAULT); }
}
extern "C" uint8_t os_crc_check_progress(void) { return crc_complete ? 100 : 0; }
extern "C" bool os_crc_check_complete(void) { return crc_complete; }
extern "C" uint32_t os_get_crc_error_count(void) { return crc_error_count; }
#endif
