#define OS_BUILD
#include "ZenOS_Internal.hpp"

#if OS_SAFETY_MPU

/* Correct Armv7-M PMSA implementation. Armv8-M PMSAv8 is intentionally
   excluded by ZenOS_Port.hpp until an RBAR/RLAR + security-state port exists. */
#undef os_mpu_init
#undef os_mpu_configure_task
#undef os_mpu_add_region
#undef os_mpu_disable
#undef os_mpu_enable

namespace {
struct MPU_Regs {
    volatile uint32_t TYPE;
    volatile uint32_t CTRL;
    volatile uint32_t RNR;
    volatile uint32_t RBAR;
    volatile uint32_t RASR;
};
static MPU_Regs* const MPU = (MPU_Regs*)0xE000ED90UL;

constexpr uint32_t CTRL_ENABLE = 1U;
constexpr uint32_t CTRL_PRIVDEFENA = 1U << 2;
constexpr uint32_t AP_FULL = 3U; /* privileged + unprivileged RW */
constexpr uint32_t AP_RO   = 6U; /* privileged + unprivileged RO */
constexpr uint32_t RASR_XN = 1UL << 28;
constexpr uint32_t RASR_C  = 1UL << 17;
constexpr uint32_t RASR_B  = 1UL << 16;
constexpr uint32_t RASR_S  = 1UL << 18;
constexpr uint32_t RASR_EN = 1UL;

static bool encode_region(uint32_t base, uint32_t size, uint32_t attrs,
                          bool xn, uint32_t& rbar, uint32_t& rasr) {
    if (size < 32U || (size & (size - 1U)) != 0U) return false;
    if ((base & (size - 1U)) != 0U) return false;
    uint32_t lg = 0;
    uint32_t n = size;
    while (n > 1U) { n >>= 1; ++lg; }
    if (lg < 5U || lg > 31U) return false;
    uint32_t ap = attrs & 7U;
    uint32_t tex = (attrs >> 8) & 7U;
    uint32_t c = (attrs >> 17) & 1U;
    uint32_t b = (attrs >> 16) & 1U;
    uint32_t s = (attrs >> 18) & 1U;
    rbar = base;
    rasr = ((lg - 1U) << 1) | (ap << 24) | (tex << 19) |
           (s << 18) | (c << 17) | (b << 16) | (xn ? RASR_XN : 0U) | RASR_EN;
    return true;
}

static bool program_region(uint32_t number, uint32_t base, uint32_t size,
                           uint32_t attrs, bool xn) {
    uint32_t rbar, rasr;
    if (number >= OS_MPU_MAX_REGIONS || !encode_region(base, size, attrs, xn, rbar, rasr)) return false;
    MPU->RNR = number;
    MPU->RBAR = rbar;
    MPU->RASR = rasr;
    return true;
}

static uint32_t largest_pow2_region(uint32_t base, uint32_t size) {
    if (!size) return 0;
    uint32_t r = 1U;
    while (r <= (size >> 1)) r <<= 1;
    while (r && (base & (r - 1U))) r >>= 1;
    return r >= 32U ? r : 0U;
}
}

extern "C" void os_mpu_disable(void) {
    MPU->CTRL = 0U;
    __asm volatile("dsb\n isb" ::: "memory");
}

extern "C" void os_mpu_enable(void) {
    MPU->CTRL = CTRL_ENABLE | CTRL_PRIVDEFENA;
    __asm volatile("dsb\n isb" ::: "memory");
}

extern "C" void os_mpu_init(void) {
    os_mpu_disable();

    /* Region 0: executable Flash, unprivileged read-only. */
    uint32_t flash = largest_pow2_region(OS_FLASH_START, OS_FLASH_SIZE);
    uint32_t ram   = largest_pow2_region(OS_RAM_START, OS_RAM_SIZE);
    if (!flash || !ram) {
        /* Never enable a partially described MPU map. */
        return;
    }
    if (!program_region(0U, OS_FLASH_START, flash, AP_RO | (1U << 17), false)) return;
    if (!program_region(1U, OS_RAM_START, ram, AP_FULL | (1U << 17) | (1U << 18), true)) return;
    /* Peripheral space: Device-like, non-executable, RW. */
    if (!program_region(2U, 0x40000000UL, 0x20000000UL,
                        AP_FULL | (2U << 19) | (1U << 18), true)) return;
    os_mpu_enable();
}

extern "C" void os_mpu_configure_task(void* tcb_ptr) {
    TCB* task = static_cast<TCB*>(tcb_ptr);
    if (!task) return;
    uint32_t size = largest_pow2_region((uint32_t)(uintptr_t)task->stack_base,
                                        task->stack_size * 4U);
    if (!size) return;
    uint32_t base = (uint32_t)(uintptr_t)task->stack_base;
    task->mpu_region_count = 0;
    if (program_region(3U, base, size, AP_FULL | (1U << 17) | (1U << 18), true)) {
        task->mpu_region_count = 1;
    }
    __asm volatile("dsb\n isb" ::: "memory");
}

extern "C" void os_mpu_add_region(void* tcb_ptr, uint32_t base, uint32_t size, uint32_t attrs) {
    TCB* task = static_cast<TCB*>(tcb_ptr);
    if (!task || task->mpu_region_count >= (OS_MPU_MAX_REGIONS - 3U)) return;
    uint32_t number = 3U + task->mpu_region_count;
    if (program_region(number, base, size, attrs, (attrs & (1UL << 28)) != 0U))
        ++task->mpu_region_count;
}

#endif /* OS_SAFETY_MPU */
