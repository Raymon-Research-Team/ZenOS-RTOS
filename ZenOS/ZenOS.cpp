/**
 * @file    ZenOS.cpp
 * @brief   ZenOS RTOS — Kernel Core (globals, init, start, PendSV, idle, tick, yield, delay)
 *
 * This file contains the kernel core that ties all modules together.
 * Modularized code lives in:
 *   ZenOS_Scheduler.cpp — O(1) bitmap scheduler, task create/control/lookup
 *   ZenOS_IPC.cpp       — Events, Mutex, C++ wrapper classes
 *   ZenOS_Safety.cpp    — Error system, stack check, fault handler, watchdog, CRC, RAM test, MPU
 *   ZenOS_Monitor.cpp   — Stack watermark, CPU usage, deadline, error log
 *
 * @author  Rahman Heidari <rahman.h22@gmail.com> — Raymon Research Team
 * @version 1.1.0
 */

#define OS_BUILD
#include "ZenOS_Internal.hpp"

/* SystemCoreClock is provided by CMSIS (system_stm32fxxx.c) */

/* Forward declarations for vector table installation */
extern "C" void os_tick(void);
extern "C" void OS_PendSV_Handler(void);
extern "C" void OS_Fault_Handler(void);

/* Only cores with a VTOR relocate the vector table at runtime; on ARMv6-M
   (Cortex-M0/M0+) there is no VTOR, the table stays in flash and this buffer
   would be dead storage (and a -Werror=unused-variable failure). */
#if OS_HAS_VTOR
static uint32_t ram_vectors[OS_VECTOR_COUNT] OS_ALIGNED(512);
#endif

/* ═══════════════ Shared Globals ═══════════════ */
/* All scheduler globals below are shared between task context, the naked
   PendSV handler and the SysTick/EXTI ISRs.  They MUST be volatile: at
   -O2 the compiler otherwise keeps them cached in registers across the
   asm boundaries (os_yield, OS_PendSV_Handler) and the scheduler then
   switches to stale TCBs — the classic "works at -O0, corrupt at -O2". */
TCB*              volatile _os_task_list    = nullptr;
uint16_t          volatile _os_task_count   = 0;
TCB*              volatile current_task = nullptr;

uint32_t          _os_idle_stack[OS_IDLE_STACK_WORDS] OS_ALIGNED(8);
volatile uint32_t tick_count = 0;

#if OS_IPC_TOOLS_EN
ECB* volatile _os_event_list = nullptr;
int16_t os_event_next_id = 0;
#endif

TCB _os_idle_tcb;
extern "C" TCB* const os_idle_tcb_ptr = &_os_idle_tcb;

volatile uint32_t _os_blocked_count   = 0;

volatile uint32_t _os_idle_ticks      = 0;
volatile bool     _os_started      = false;

volatile uint32_t os_ready_bitmap = 0;
TCB* volatile os_pq_head[32] = {nullptr};

volatile uint32_t _os_syst_rvr_normal = 0;
static uint32_t fault_stack[OS_FAULT_STACK_WORDS] OS_ALIGNED(8);

volatile uint32_t os_safe_depth = 0;

/* ── Microsecond time domain (see _os_time_fold in ZenOS_Internal.hpp) ──
   The DWT cycle counter is 32-bit and wraps (~59.6 s at 72 MHz), so raw
   CYCCNT reads cannot measure beyond one wrap period.  These three values
   form a wrap-safe µs accumulator:
     _os_us_cycles_pending — CYCCNT value at the last fold (wrap baseline)
     _os_us_remainder      — fractional µs not yet convertible, carried in
                            cycle·µs units (cycles × 10^6) so per-fold
                            truncation cannot accumulate into drift
     _os_us_accumulated    — µs since os_init()/_os_time_reset()
   Conversion is exact for ANY SystemCoreClock via µs = cycles × 10^6 /
   SystemCoreClock, and a clock change automatically applies only to
   subsequent windows.  _os_time_fold() is the single writer (os_tick,
   _os_time_reset and the lazy path in os_get_us all route through it);
   _os_time_reset() reinitializes the domain.  All state is guarded by
   os_critical_enter/exit at every call site. */
volatile uint32_t _os_us_cycles_pending = 0;
volatile uint32_t _os_us_remainder      = 0;
volatile uint32_t _os_us_accumulated    = 0;

volatile uint32_t _os_error_total = 0;
volatile OSError  _os_error_last  = OSError::NONE;
volatile uint32_t _os_error_expected     = 0;
volatile uint32_t _os_error_expect_depth = 0;
volatile uint32_t _os_wdg_reset_count      = 0;
volatile uint32_t _os_stack_recovery_count = 0;


/* ═══════════════ Critical Section ═══════════════ */
extern "C" uint32_t os_critical_enter(void) {
    uint32_t old;
    __asm volatile("mrs %0, PRIMASK\n cpsid i\n" : "=r"(old) :: "memory");
    return old;
}

extern "C" void os_critical_exit(uint32_t old) {
    __asm volatile("msr PRIMASK, %0" :: "r"(old) : "memory");
}


/* ═══════════════ Delays ═══════════════ */
extern "C" void os_delay_us(uint32_t us) {
    if (us == 0) return;

    if (os_safe_depth > 0) {
#if OS_HAS_CYCLE_COUNTER
        /* Opt5: DWT busy-wait — exit once the cycle counter passes the deadline */
        uint32_t cycles = (uint32_t)((uint64_t)us * (SystemCoreClock / 1000000UL));
        uint32_t deadline = OS_DWT_CYCCNT + cycles;
        while ((int32_t)(OS_DWT_CYCCNT - deadline) < 0)
            __asm volatile("nop");
#else
        /* Cortex-M0/M0+ without DWT: calibrated NOP loop.
           Each iteration is ~4 cycles (nop + loop overhead).
           SystemCoreClock/4000000 gives iterations for ~1µs at 4 cycles/iter. */
        uint32_t n = (uint32_t)((uint64_t)us * (SystemCoreClock / 4000000UL));
        if (n == 0) n = 1;
        __asm volatile(
            "1: subs %0, #1\n"
            "   bne 1b\n"
            : "+r"(n) :: "cc"
        );
#endif
        return;
    }

    if (us >= 1000UL) { os_delay_ms(us / 1000UL); return; }

    if (_os_syst_rvr_normal == 0) {
        volatile uint32_t n = us;
        while (n--) {
            for (volatile uint32_t i = 0; i < 10; i++)
                __asm volatile("nop");
        }
        return;
    }

    volatile uint32_t start = os_get_us();
    while ((os_get_us() - start) < us) __asm volatile("nop");
}

extern "C" void os_delay_ms(uint32_t ms) {
    /* Never block from an ISR: os_safe_depth only catches the RAII guards,
       _os_in_isr() catches every exception context. */
    if (os_safe_depth > 0 || _os_in_isr()) { _os_report_error(OSError::SAFE_DELAY_MS); return; }
    if (ms == 0) return;

    /* BUG-004 fix: before os_start() there is no scheduler and PendSV still
       points at the startup weak handler (infinite loop).  os_yield() from a
       pre-start delay therefore hung the boot forever.  Busy-wait on the DWT
       µs domain instead (it is initialised by os_init and needs no tick). */
    if (!_os_started) {
        while (ms--) {
            uint32_t start = os_get_us();
            while ((os_get_us() - start) < 1000UL) __asm volatile("nop");
        }
        return;
    }

    uint32_t delay_ticks = _os_ms_to_ticks(ms);
    uint32_t cs = os_critical_enter();
    _os_block_current(TaskState::BLOCKED, nullptr, 0, delay_ticks);
    os_critical_exit(cs);
    os_yield();
}

/* Opt1: os_yield — direct asm, no function call overhead (~20 cycles saved) */
extern "C" void os_yield(void) {
    __asm volatile(
        "mrs   r0, PRIMASK\n"
        "cpsid i\n"
        "ldr   r1, =os_safe_depth\n"
        "ldr   r1, [r1]\n"
        "cmp   r1, #0\n"
        "bne   1f\n"
        "ldr   r1, =current_task\n"
        "ldr   r1, [r1]\n"
        "cmp   r1, #0\n"
        "beq   1f\n"
        "ldr   r2, =tick_count\n"
        "ldr   r2, [r2]\n"
        "str   r2, [r1, #" OS_STR(OS_OFF_LAST_YIELD_TICK) "]\n"
        "1:\n"
        "msr   PRIMASK, r0\n"
        /* STR directly to ICSR PendSVSet bit — no read-modify-write */
        "ldr   r0, =0xE000ED04\n"
        "ldr   r1, =0x10000000\n"
        "str   r1, [r0]\n"
        : /* no outputs */
        : /* no inputs */
        : "r0", "r1", "r2", "cc", "memory"
    );
}


/* ═══════════════ Tick Handler ═══════════════ */
extern "C" void os_tick(void) {
    /* ═══════ C2 fix: one critical section for the whole tick path ═══════
       _os_tickless_process() and _os_stack_check_all() mutate global
       scheduler state (priority queues, bitmap, _os_blocked_count). They must
       run with interrupts disabled, otherwise a higher-priority ISR
       (e.g. EXTI calling os_event_signal_from_isr) can corrupt that state
       concurrently with the µs sampler below. */
    uint32_t cs = os_critical_enter();

    _os_time_sample();

    tick_count++;

    if (current_task == &_os_idle_tcb) _os_idle_ticks++;
    else if (current_task && current_task->state == TaskState::RUNNING) {
        current_task->cpu_ticks++;
#if OS_MONITORING_EN
        /* Stack watermark: track lowest SP seen */
        if ((uint32_t)current_task->stack_top < (uint32_t)current_task->peak_sp)
            current_task->peak_sp = current_task->stack_top;
#endif
    }

#if OS_SAFETY_SOFT_WATCHDOG_EN
    /* Check every 1ms (OS_TICKS_PER_MS ticks) to reduce overhead */
    if ((tick_count % OS_TICKS_PER_MS) == 0 &&
        current_task && current_task != &_os_idle_tcb &&
        current_task->state == TaskState::RUNNING) {
        uint32_t elapsed = tick_count - current_task->last_yield_tick;
        uint32_t max_ticks = OS_SAFETY_SOFT_WDG_TIMEOUT_MS * OS_TICKS_PER_MS;
        if (elapsed > max_ticks) {
            _os_report_error(OSError::TASK_STUCK);
            _os_wdg_reset_count++;
            /* O(1) scheduler: remove from pq before reset */
            _os_pq_remove(current_task);
            if (current_task->wdg_retries < OS_SAFETY_TASK_MAX_RECOVERY) {
                current_task->wdg_retries++;
                _os_reset_task_internal(current_task);
            } else {
                current_task->state = TaskState::INACTIVE;
            }
            current_task = nullptr;
        }
    }
#endif

#if OS_MONITORING_EN
    /* Deadline monitoring — only flag the miss; PendSV handles action */
    if (current_task && current_task != &_os_idle_tcb &&
        current_task->state == TaskState::RUNNING &&
        current_task->deadline_ticks > 0) {
        uint32_t elapsed = tick_count - current_task->last_yield_tick;
        if (elapsed > current_task->deadline_ticks) {
            current_task->deadline_miss_count++;
            _os_report_error(OSError::DEADLINE_MISS);
            /* Do NOT reset/disable task from ISR context —
               only PendSV (lowest priority) should modify current_task.
               The miss is recorded; higher-level code can react. */
        }
    }
#endif

    if (_os_blocked_count > 0) {
        for (TCB* task = _os_task_list; task; task = task->next) {
            if (task->state != TaskState::BLOCKED) continue;

            if (task->delay_ticks > 0) {
                task->delay_ticks--;
                if (task->delay_ticks == 0) {
                    _os_wake_on_delay_expiry(task);
                }
            } else if (task->block_timeout > 0) {
                task->block_timeout--;
                if (task->block_timeout == 0) {
                    _os_wake_on_timeout_expiry(task);
                }
            }
        }
    }

    /* Stack check mutates the same scheduler structures, so it stays
       inside the critical section too. */
    if ((tick_count & 0x0F) == 0) _os_stack_check_all();

    os_critical_exit(cs);
    /* ═══════ end C2 fix ═══════ */

    /* DSB: all tick processing (task state changes, wakeups, stack checks)
       must be visible in RAM before PendSV runs and reads them. */
    __asm volatile("dsb" ::: "memory");
    /* Always fire PendSV — scheduler handles release-time gating,
       priority selection, and round-robin rotation. */
    OS_SCB_ICSR = OS_ICSR_PENDSVSET_Msk;
}


/* ═══════════════ Tickless Idle ═════════════════════════════════ */
#if OS_KERNEL_TICKLESS_IDLE_EN
static bool os_idle_tickless(void) {
    uint32_t saved_rvr = _os_syst_rvr_normal;
    if (saved_rvr == 0) return false;

    /* BUG-006 fix (root cause of the 1 ms + tickless "tick_count stays 0"
       freeze): when a task is blocked, this function must NOT touch SysTick
       at all.  The previous code disabled SysTick, zeroed CVR and re-armed it
       on EVERY idle iteration.  The HAL time base (TIM1) fires at the same
       1 ms rate and wakes WFI long before SysTick's reload elapses, so the
       counter was restarted from zero every time and the SysTick exception
       never fired — tick_count stayed 0 forever and blocked tasks were never
       woken.  Sleeping with the untouched normal reload lets the hardware
       deliver every tick, and os_tick() does the blocked-timeout bookkeeping
       with exact per-tick granularity.  Freeze/extend only for pure idle
       sleep (nothing blocked), where no timeout depends on the tick. */
    if (_os_blocked_count > 0) return false;

    uint32_t ticks_per_os_tick = saved_rvr + 1;
    uint32_t sleep = 0;
    uint32_t hw_sleep = 0;
    uint32_t wake_csr = 0;

    {
        uint32_t cs = os_critical_enter();
        OS_SYST_CSR = 0;

        uint32_t max_sleep = 0x00FFFFFFUL / ticks_per_os_tick;
        if (max_sleep == 0) max_sleep = 1;
        sleep = max_sleep;

        if (_os_blocked_count > 0) {
            for (TCB* t = _os_task_list; t; t = t->next) {
                if (t->state == TaskState::BLOCKED) {
                    if (t->delay_ticks > 0 && t->delay_ticks < sleep)
                        sleep = t->delay_ticks;
                    if (t->block_timeout > 0 && t->block_timeout < sleep)
                        sleep = t->block_timeout;
                }
            }
        }

        if (sleep <= 1) {
            /* BUG-003 fix: sleep windows of 0/1 ticks cannot use the extended
               reload — restore the NORMAL reload before returning so SysTick
               keeps running at its configured rate (a previous freeze with a
               huge RVR must never leak past this function). */
            OS_SYST_CVR = 0;
            OS_SYST_RVR = saved_rvr;
            OS_SYST_CSR = 0x07;
            os_critical_exit(cs);
            return false;
        }

        uint64_t hw64 = (uint64_t)ticks_per_os_tick * sleep;
        if (hw64 > 0x00FFFFFFULL) {
            sleep = 0x00FFFFFFUL / ticks_per_os_tick;
            if (sleep <= 1) {
                /* Same normal-reload restore as above */
                OS_SYST_CVR = 0;
                OS_SYST_RVR = saved_rvr;
                OS_SYST_CSR = 0x07;
                os_critical_exit(cs);
                return false;
            }
            hw64 = (uint64_t)ticks_per_os_tick * sleep;
        }
        hw_sleep = (uint32_t)hw64;

        OS_SYST_CVR = 0;
        OS_SYST_RVR = hw_sleep - 1;
        OS_SYST_CSR = 0x07;

        /* Sleep loop: keep sleeping until SysTick expires.
           Peripheral interrupts (e.g. HAL TIM1 timebase at 1 ms) can wake
           the CPU from WFI before SysTick expires.  On Cortex-M, WFI returns
           when *any* interrupt is pending — even if the interrupt priority is
           lower than the current execution priority.  If we simply broke out
           of WFI on every peripheral wake, the elapsed sub-tick cycles were
           divided away by integer truncation (skip == 0 at 1 ms tick), and the
           next call to os_idle_tickless() would DISABLE SysTick before it could
           fire — creating an infinite loop where SysTick never expires and
           tasks never wake.

           Fix: on a peripheral wake (COUNTFLAG clear) we temporarily freeze
           SysTick so CVR retains its countdown position, let the pending ISR
           run, then resume SysTick and re-enter WFI.  Only break out when
           SysTick actually expires (COUNTFLAG set). */
        while (true) {
            __asm volatile("dsb" ::: "memory");
            __asm volatile("wfi");
            __asm volatile("isb" ::: "memory");

            wake_csr = OS_SYST_CSR;
            if ((wake_csr & OS_SYST_CSR_COUNTFLAG_Msk) != 0) {
                /* SysTick expired — normal wake path */
                break;
            }
            /* Peripheral woke us before SysTick expired.
               Freeze SysTick so CVR keeps its countdown position,
               let the pending ISR run, then resume. */
            OS_SYST_CSR = 0;
            os_critical_exit(cs);
            cs = os_critical_enter();
            OS_SYST_CSR = 0x07;
        }

        os_critical_exit(cs);
    }

    {
        uint32_t cs = os_critical_enter();
        uint32_t cvr_val = OS_SYST_CVR;
        OS_SYST_CSR = 0;

        /* BUG-001 fix: two exclusive wake cases.
           1) COUNTFLAG was set at wake: SysTick expired during the sleep.
              os_tick() has already counted that wake tick once the critical
              section above exited, so only the remaining (sleep - 1) ticks
              are credited here — never the full interval, or every wake tick
              would be counted twice.
           2) COUNTFLAG was clear: a peripheral interrupt woke us before
              expiry, the counter has not reloaded, and CVR still reflects
              the true elapsed hardware counts. Derive the elapsed ticks from
              CVR as before. */
        uint32_t skip = 0;
        if ((wake_csr & OS_SYST_CSR_COUNTFLAG_Msk) != 0) {
            skip = (sleep > 0) ? (sleep - 1) : 0;
        }
        else if (cvr_val < hw_sleep) {
            uint32_t elapsed_hw = (hw_sleep - 1) - cvr_val;
            skip = elapsed_hw / ticks_per_os_tick;
        }
        if (skip > 0) {
            /* C3 fix: apply the skipped time right now, inside the CS —
               forward tick_count and wake expired tasks. Deferring this
               to the next SysTick loses time whenever another IRQ (e.g.
               the HAL TIM1 time base) wakes us repeatedly before that
               SysTick fires, because tick_skip would be overwritten. */
            _os_idle_ticks += skip;
            /* Wake the scheduler so an expired task runs immediately
               (PendSV re-evaluates the release gate; see _os_pq_next). */
            OS_SCB_ICSR = OS_ICSR_PENDSVSET_Msk;
        }
        /* Known residual edge (documented, not yet observed): if a peripheral
           IRQ wakes us and SysTick expires inside the narrow window between
           the wake_csr sample and this CVR read, the handler re-runs before
           this block and up to one extra tick of the sleep interval can be
           lost. Window is a few CPU cycles vs. a >= 1 ms interval. */

        OS_SYST_CVR = 0;
        OS_SYST_RVR = saved_rvr;
        OS_SYST_CSR = 0x07;
        os_critical_exit(cs);
    }

    return true;
}

/* BUG-003 note: after the release-gate fix the system schedules correctly
   again, but a resume helper that restored the normal SysTick reload from
   the idle loop made the board hang in WFI (no UART at all): touching
   SysTick from task context after the wake races with the already-pended
   PendSV.  The freeze/unfreeze is therefore kept strictly inside
   os_idle_tickless(), and the idle loop only guarantees memory ordering
   (DSB) before its own WFI. */

/* Forward declaration for tickless processing (in ZenOS_Scheduler.cpp) */
extern bool _os_tickless_process(uint32_t skip);
#endif



/* ═══════════════ Idle ═══════════════ */
static void os_idle_task(void) {
    while (1) {
#if OS_SAFETY_HW_WATCHDOG_EN
        os_hw_watchdog_check();  /* feed if healthy */
#endif
#if OS_SAFETY_CRC_EN
        os_crc_check_step();    /* check ROM in background */
#endif
#if OS_KERNEL_TICKLESS_IDLE_EN
        os_idle_tickless();
#endif
        __asm volatile("dsb");
        __asm volatile("wfi");
    }
}

/* ═══════════════ Init ═══════════════ */
extern "C" void os_init(void) {
    /* SystemCoreClock is set by SystemInit() (CubeMX) before main(). */

    /* os_event_next_id only exists when the event feature is compiled in;
       ZenOS.cpp is built for every configuration, so the reset must be
       guarded or OS_IPC_TOOLS_EN=0 fails to compile. */
#if OS_IPC_TOOLS_EN
    os_event_next_id = 0;
#endif
    _os_task_list = nullptr; _os_task_count = 0; current_task = nullptr;
    tick_count = 0; _os_blocked_count = 0;

    /* _os_idle_tcb: safe defaults for fault recovery before os_start */
    _os_idle_tcb.id = 255; _os_idle_tcb.name = "idle"; _os_idle_tcb.entry = nullptr;
    _os_idle_tcb.priority = 0; _os_idle_tcb.base_priority = 0;
    _os_idle_tcb.state = TaskState::INACTIVE;
    _os_idle_tcb.next = nullptr;
    os_safe_depth = 0; _os_error_total = 0; _os_error_expected = 0;
    _os_error_expect_depth = 0; _os_error_last = OSError::NONE;
    _os_idle_ticks = 0; _os_wdg_reset_count = 0; _os_stack_recovery_count = 0;
    _os_syst_rvr_normal = 0;
    /* O(1) scheduler: initialize ALL priority bitmap and queues
       (including extension storage for >32 priorities) */
    _os_priority_queues_init();


#if OS_HAS_CYCLE_COUNTER
    OS_COREDEBUG_DEMCR |= OS_COREDEM_TRCENA;
    OS_DWT_CYCCNT = 0;
    OS_DWT_CTRL  |= OS_DWT_CYCCNTENA;
#endif


    /* µs time domain: start the extension from zero exactly when the DWT
       counter resets.  MUST run after the DWT block above (which zeroes
       CYCCNT). */
    _os_time_reset();
#if OS_SAFETY_MPU_EN
    os_mpu_init();
#endif
#if OS_SAFETY_CRC_EN
    /* Capture the expected code-image CRC at boot so os_crc_check_step()
       (called from the idle task) has a valid reference.  Without this
       call the checker compared against a zero baseline and reported a
       false CRC fault on the first idle iteration. */
    os_crc_init();
#endif
	
}


/* ═══════════════ Microsecond Time Extension (DWT wrap safety) ═══════════════
   The DWT CYCCNT register is 32-bit: at 72 MHz it wraps every ~59.6 s,
   at 480 MHz every ~8.9 s.  A bare read of CYCCNT therefore cannot
   measure beyond one wrap period.  These two functions keep a 32-bit
   µs accumulator that survives wraparound indefinitely:

   _os_time_sample()  — folds the DWT cycles elapsed since the previous
                       sample into _os_us_accumulated.  Conversion is exact
                       for any SystemCoreClock via cycles × 10^6 / clock,
                       and the carried remainder prevents truncation from
                       accumulating into drift across folds.
   _os_time_reset()   — reinitializes the domain after os_init() zeroes
                       CYCCNT, and after SystemCoreClock updates.

   Every caller holds the os_critical section — the state is touched
   from task context, SysTick and the tickless-idle wake path. */
void _os_time_reset(void) {
    uint32_t cs = os_critical_enter();
    _os_us_accumulated = 0;
    _os_us_remainder   = 0;
#if OS_HAS_CYCLE_COUNTER
    _os_us_cycles_pending = OS_DWT_CYCCNT;
#else
    _os_us_cycles_pending = 0;
#endif
    os_critical_exit(cs);
}

void _os_time_sample(void) {
#if OS_HAS_CYCLE_COUNTER
    _os_time_fold();
#else
    /* No DWT: the µs domain is derived from tick_count directly in
       os_get_us(); nothing to sample. */
#endif
}

/* ═══════════════ Start ═══════════════ */
extern "C" void os_start(void) {
    _os_idle_tcb.id = 255; _os_idle_tcb.name = "idle"; _os_idle_tcb.entry = os_idle_task;
    _os_idle_tcb.priority = 0; _os_idle_tcb.state = TaskState::READY;
    /* Use the configured idle stack size, NOT a hardcoded 64, so that
       changing OS_IDLE_STACK_WORDS in ZenOS_Config.hpp/Port.hpp actually
       takes effect.  A mismatch here would make _os_stack_check_all()
       report the idle task as overflowing (or under-check it). */
    _os_idle_tcb.stack_base = _os_idle_stack; _os_idle_tcb.stack_size = OS_IDLE_STACK_WORDS;
    _os_idle_tcb.period_ticks = 0; _os_idle_tcb.next_run_time = 0;
    _os_idle_tcb.delay_ticks = 0; _os_idle_tcb.blocking_on = nullptr;
    _os_idle_tcb.block_timeout = 0; _os_idle_tcb.wait_result = 0;
    _os_idle_tcb.last_yield_tick = tick_count;
    _os_idle_tcb.mutex_nesting = 0; _os_idle_tcb.base_priority = 0;
    _os_idle_tcb.mutex_held_count = 0;
    _os_idle_tcb.next = _os_task_list;
    _os_task_list = &_os_idle_tcb;
    _os_stack_init(&_os_idle_tcb);
    /* Priority 0 is reserved: idle is never enqueued.  PendSV falls back to
       os_idle_tcb_ptr when no priority queue has an eligible task.  This call
       is a no-op guard — it must NOT reset scheduler state. */
    _os_pq_add(&_os_idle_tcb);


#if OS_HAS_VTOR
    {
        uint32_t* fv = (uint32_t*)OS_SCB_VTOR;
        for (uint32_t i = 0; i < OS_VECTOR_COUNT; i++)
            ram_vectors[i] = fv[i];

        ram_vectors[OS_PENDSV_VECTOR_INDEX]     = (uint32_t)OS_PendSV_Handler;
        ram_vectors[OS_SYSTICK_VECTOR_INDEX]    = (uint32_t)os_tick;
        ram_vectors[OS_HARDFAULT_VECTOR_INDEX]  = (uint32_t)OS_Fault_Handler;
        ram_vectors[OS_MEMMANAGE_VECTOR_INDEX]  = (uint32_t)OS_Fault_Handler;
        ram_vectors[OS_BUSFAULT_VECTOR_INDEX]   = (uint32_t)OS_Fault_Handler;
        ram_vectors[OS_USAGEFAULT_VECTOR_INDEX] = (uint32_t)OS_Fault_Handler;
        /* All other IRQ handlers (TIM, UART, SPI, EXTI, etc.) are preserved
           from the flash vector table copy above — works with any timer
           or peripheral the user configures in CubeMX. */
        OS_SCB_VTOR = (uint32_t)ram_vectors;
    }
#else
    /* Cortex-M0/M0+ has no VTOR — PendSV and fault handlers must be
       installed via weak symbol override in the startup file.
       The startup_stm32f0xx.s / startup_stm32g0xx.s already provides
       weak aliases; our OS_PendSV_Handler / OS_Fault_Handler symbols
       override them at link time. */
#endif

    OS_SYST_CSR = 0; OS_SYST_CVR = 0;
    uint32_t reload = (SystemCoreClock / 1000000UL) * OS_KERNEL_TICK_PERIOD_US;
    if (reload == 0) reload = 1;
    if (reload > 0x00FFFFFFUL) reload = 0x00FFFFFFUL;
    _os_syst_rvr_normal = reload - 1;
    OS_SYST_RVR = reload - 1;

    OS_PENDSV_PRIO  = 0xFE;
    OS_SYSTICK_PRIO = 0xFF;

    /* PendSV at 0xFE is above all common IRQ priorities (0, 5, 0xFF)
       No conflict check needed — PendSV always preempts them. */

    {
        uint32_t msp_val = (uint32_t)(fault_stack + OS_FAULT_STACK_WORDS);
        msp_val &= ~7UL;
        __asm volatile("msr msp, %0" :: "r"(msp_val));
    }

    __asm volatile("msr psp, %0" :: "r"(_os_idle_tcb.stack_top));
    __asm volatile("msr control, %0" :: "r"(0x02));
    __asm volatile("isb");

    /* BUG-005 fix (root cause of the 1 ms + tickless "tasks never run" bug):
       _os_started MUST be true before PendSV is pended and interrupts are
       enabled.  The ICSR PendSVSET write takes effect the instant IRQs are
       unmasked, and PendSV switches straight to the highest-priority ready
       task — the boot thread never comes back (its context is dropped on
       first_run).  With the flag set AFTER _os_hw_enable_irq(), that store
       was dead code: _os_started stayed 0 forever, so os_delay_ms() (and
       every other blocking call) took the not-started path and tasks never
       blocked/yielded — with the 100 µs default tick the timing tests
       masked it, but at 1 ms + tickless idle the scheduler starved. */
    _os_started = true;
    OS_SYST_CSR = 0x07;
    OS_SCB_ICSR = OS_ICSR_PENDSVSET_Msk;
    _os_hw_enable_irq();

    while (1) __asm volatile("wfi");
}

/* ═══════════════ PendSV Handler — Bitmap + Period Gate Scheduler ═══════════════
   Uses the priority bitmap first, then checks periodic release eligibility
   inside the selected priority queues. No fixed task-count limit is used. ═══════ */
/* ──────────────────────────────────────────────────────────────────────────────
 *  PendSV Context Switch — FPU-Aware, Multi-Architecture
 *  ==========================================================================
 *  Two compile-time variants exist:
 *    OS_HAS_FPU_HW = 1: FPU save/restore via vstmdb/vldmia (Cortex-M4F/M7F)
 *    OS_HAS_FPU_HW = 0: Core registers only (Cortex-M0/M0+/M3)
 *
 *  The FPU variant detects lazy stacking via EXC_RETURN bit 4 and
 *  conditionally saves/restores S16-S31 + FPSCR.
 *
 *  EXC_RETURN bit 4:
 *    0 = FPU context in exception frame (S0-S15 + FPSCR stacked)
 *    1 = No FPU context in exception frame
 * ──────────────────────────────────────────────────────────────────────────── */

#if OS_HAS_FPU_HW
/* ════════════ FPU variant: saves S16-S31 when task used FPU ════════════ */
extern "C" OS_NAKED OS_USED void OS_PendSV_Handler(void) {
    __asm volatile(
        ".syntax unified\n"
        ".thumb\n"
        "mrs r0, psp\n"
        "ldr r1, =current_task\n"
        "ldr r1, [r1]\n"
        "cmp r1, #0\n"
        "bne 1f\n"
        "b first_run\n"
        "1:\n"

        /* ── FPU state: check EXC_RETURN bit 4 ── */
        "tst lr, #0x10\n"
        "bne 2f\n"
        "vstmdb r0!, {s16-s31}\n"
        "vmrs r2, fpscr\n"
        "str r2, [r0, #-4]!\n"
        "2:\n"

        /* Save r4-r11 */
        "stmdb r0!, {r4-r11}\n"
        "str r0, [r1, #" OS_STR(OS_OFF_STACK_TOP) "]\n"

        /* RUNNING -> READY */
        "ldrb r2, [r1, #" OS_STR(OS_OFF_STATE) "]\n"
        "cmp r2, #2\n"
        "bne 3f\n"
        "movs r2, #1\n"
        "strb r2, [r1, #" OS_STR(OS_OFF_STATE) "]\n"
        "3:\n"

        "ldr r8, =tick_count\n"
        "ldr r9, =os_idle_tcb_ptr\n"

        /* Scheduler */
        "push {lr}\n"
        "bl _os_pq_next\n"
        "mov r5, r0\n"
        "bl _os_pq_rotate\n"
        "pop {lr}\n"

        "cmp r5, #0\n"
        "beq fallback_idle\n"

        "ldr r1, =current_task\n"
        "str r5, [r1]\n"
        "movs r6, #2\n"
        "strb r6, [r5, #" OS_STR(OS_OFF_STATE) "]\n"
        "ldr r7, [r5, #" OS_STR(OS_OFF_PERIOD_TICKS) "]\n"
        "cmp r7, #0\n"
        "beq 4f\n"
        "ldr r6, [r8]\n"
        "adds r6, r6, r7\n"
        "str r6, [r5, #" OS_STR(OS_OFF_NEXT_RUN_TIME) "]\n"
        "b 5f\n"
        "4:\n"
        "ldr r6, [r8]\n"
        "adds r6, r6, #1\n"
        "str r6, [r5, #" OS_STR(OS_OFF_NEXT_RUN_TIME) "]\n"
        "5:\n"
        "ldr r0, [r5, #" OS_STR(OS_OFF_STACK_TOP) "]\n"
        "b restore_ctx\n"

        "fallback_idle:\n"
        "ldr r5, [r9]\n"
        "ldr r1, =current_task\n"
        "str r5, [r1]\n"
        "movs r6, #2\n"
        "strb r6, [r5, #" OS_STR(OS_OFF_STATE) "]\n"
        "ldr r6, [r8]\n"
        "adds r6, r6, #1\n"
        "str r6, [r5, #" OS_STR(OS_OFF_NEXT_RUN_TIME) "]\n"
        "ldr r0, [r5, #" OS_STR(OS_OFF_STACK_TOP) "]\n"
        "b restore_ctx\n"

        "first_run:\n"
        "ldr r8, =tick_count\n"
        "ldr r9, =os_idle_tcb_ptr\n"
        "push {lr}\n"
        "bl _os_pq_next\n"
        "mov r5, r0\n"
        "bl _os_pq_rotate\n"
        "pop {lr}\n"

        "cmp r5, #0\n"
        "beq fallback_idle_first\n"
        "ldr r1, =current_task\n"
        "str r5, [r1]\n"
        "movs r6, #2\n"
        "strb r6, [r5, #" OS_STR(OS_OFF_STATE) "]\n"
        "ldr r7, [r5, #" OS_STR(OS_OFF_PERIOD_TICKS) "]\n"
        "cmp r7, #0\n"
        "beq 6f\n"
        "ldr r6, [r8]\n"
        "adds r6, r6, r7\n"
        "str r6, [r5, #" OS_STR(OS_OFF_NEXT_RUN_TIME) "]\n"
        "b 7f\n"
        "6:\n"
        "ldr r6, [r8]\n"
        "adds r6, r6, #1\n"
        "str r6, [r5, #" OS_STR(OS_OFF_NEXT_RUN_TIME) "]\n"
        "7:\n"
        "ldr r0, [r5, #" OS_STR(OS_OFF_STACK_TOP) "]\n"
        "b restore_ctx\n"

        "fallback_idle_first:\n"
        "ldr r5, [r9]\n"
        "ldr r1, =current_task\n"
        "str r5, [r1]\n"
        "movs r6, #2\n"
        "strb r6, [r5, #" OS_STR(OS_OFF_STATE) "]\n"
        "ldr r0, [r5, #" OS_STR(OS_OFF_STACK_TOP) "]\n"

        "restore_ctx:\n"
#if OS_SAFETY_MPU_EN
        "ldr   r6, [r9]\n"
        "cmp   r5, r6\n"
        "beq   8f\n"
        "mrs   r6, CONTROL\n"
        "orr   r6, r6, #0x01\n"
        "msr   CONTROL, r6\n"
        "isb\n"
        "push  {r0-r3, lr}\n"
        "mov   r0, r5\n"
        "bl    os_mpu_configure_task\n"
        "pop   {r0-r3, lr}\n"
        "b     9f\n"
        "8:\n"
        "mrs   r6, CONTROL\n"
        "bic   r6, r6, #0x01\n"
        "msr   CONTROL, r6\n"
        "isb\n"
        "9:\n"
#endif

        "ldmia r0!, {r4-r11}\n"

        /* FPU restore: check EXC_RETURN bit 4 for new task */
        "tst lr, #0x10\n"
        "bne 10f\n"
        /* r0 points to start of FPU save area.  Save layout (low→high):
           [r0+0] FPSCR (4 bytes), [r0+4..r0+67] S16-S31 (64 bytes).
           The save side did: vstmdb r0!,{s16-s31} then str fpscr,[r0,#-4]!
           so FPSCR is at the lowest address. */
        "ldr  r2, [r0]\n"
        "vmsr fpscr, r2\n"
        /* ISB: FPSCR writes affect subsequent FP instruction behavior and
           must complete before S16-S31 are reloaded / task resumes. */
        "isb\n"
        "add  r0, r0, #4\n"  /* advance past FPSCR to S16-S31 */
        "vldmia r0!, {s16-s31}\n"  /* r0 now points past the whole FPU frame */
        "10:\n"

        "msr psp, r0\n"
        "isb\n"
        "bx lr\n"
        ".ltorg\n"
    );
}

#else
/* ════════════ Non-FPU variant: core registers only (no FP instructions) ════════════ */
extern "C" OS_NAKED OS_USED void OS_PendSV_Handler(void) {
    __asm volatile(
        ".syntax unified\n"
        ".thumb\n"
        "mrs r0, psp\n"
        "ldr r1, =current_task\n"
        "ldr r1, [r1]\n"
        "cmp r1, #0\n"
        "bne 1f\n"
        "b first_run\n"
        "1:\n"

        /* Save r4-r11 */
        "stmdb r0!, {r4-r11}\n"
        "str r0, [r1, #" OS_STR(OS_OFF_STACK_TOP) "]\n"

        /* RUNNING -> READY */
        "ldrb r2, [r1, #" OS_STR(OS_OFF_STATE) "]\n"
        "cmp r2, #2\n"
        "bne 3f\n"
        "movs r2, #1\n"
        "strb r2, [r1, #" OS_STR(OS_OFF_STATE) "]\n"
        "3:\n"

        "ldr r8, =tick_count\n"
        "ldr r9, =os_idle_tcb_ptr\n"

        /* Scheduler */
        "push {lr}\n"
        "bl _os_pq_next\n"
        "mov r5, r0\n"
        "bl _os_pq_rotate\n"
        "pop {lr}\n"

        "cmp r5, #0\n"
        "beq fallback_idle\n"

        "ldr r1, =current_task\n"
        "str r5, [r1]\n"
        "movs r6, #2\n"
        "strb r6, [r5, #" OS_STR(OS_OFF_STATE) "]\n"
        "ldr r7, [r5, #" OS_STR(OS_OFF_PERIOD_TICKS) "]\n"
        "cmp r7, #0\n"
        "beq 4f\n"
        "ldr r6, [r8]\n"
        "adds r6, r6, r7\n"
        "str r6, [r5, #" OS_STR(OS_OFF_NEXT_RUN_TIME) "]\n"
        "b 5f\n"
        "4:\n"
        "ldr r6, [r8]\n"
        "adds r6, r6, #1\n"
        "str r6, [r5, #" OS_STR(OS_OFF_NEXT_RUN_TIME) "]\n"
        "5:\n"
        "ldr r0, [r5, #" OS_STR(OS_OFF_STACK_TOP) "]\n"
        "b restore_ctx\n"

        "fallback_idle:\n"
        "ldr r5, [r9]\n"
        "ldr r1, =current_task\n"
        "str r5, [r1]\n"
        "movs r6, #2\n"
        "strb r6, [r5, #" OS_STR(OS_OFF_STATE) "]\n"
        "ldr r6, [r8]\n"
        "adds r6, r6, #1\n"
        "str r6, [r5, #" OS_STR(OS_OFF_NEXT_RUN_TIME) "]\n"
        "ldr r0, [r5, #" OS_STR(OS_OFF_STACK_TOP) "]\n"
        "b restore_ctx\n"

        "first_run:\n"
        "ldr r8, =tick_count\n"
        "ldr r9, =os_idle_tcb_ptr\n"
        "push {lr}\n"
        "bl _os_pq_next\n"
        "mov r5, r0\n"
        "bl _os_pq_rotate\n"
        "pop {lr}\n"

        "cmp r5, #0\n"
        "beq fallback_idle_first\n"
        "ldr r1, =current_task\n"
        "str r5, [r1]\n"
        "movs r6, #2\n"
        "strb r6, [r5, #" OS_STR(OS_OFF_STATE) "]\n"
        "ldr r7, [r5, #" OS_STR(OS_OFF_PERIOD_TICKS) "]\n"
        "cmp r7, #0\n"
        "beq 6f\n"
        "ldr r6, [r8]\n"
        "adds r6, r6, r7\n"
        "str r6, [r5, #" OS_STR(OS_OFF_NEXT_RUN_TIME) "]\n"
        "b 7f\n"
        "6:\n"
        "ldr r6, [r8]\n"
        "adds r6, r6, #1\n"
        "str r6, [r5, #" OS_STR(OS_OFF_NEXT_RUN_TIME) "]\n"
        "7:\n"
        "ldr r0, [r5, #" OS_STR(OS_OFF_STACK_TOP) "]\n"
        "b restore_ctx\n"

        "fallback_idle_first:\n"
        "ldr r5, [r9]\n"
        "ldr r1, =current_task\n"
        "str r5, [r1]\n"
        "movs r6, #2\n"
        "strb r6, [r5, #" OS_STR(OS_OFF_STATE) "]\n"
        "ldr r0, [r5, #" OS_STR(OS_OFF_STACK_TOP) "]\n"

        "restore_ctx:\n"
#if OS_SAFETY_MPU_EN
        "ldr   r6, [r9]\n"
        "cmp   r5, r6\n"
        "beq   8f\n"
        "mrs   r6, CONTROL\n"
        "orr   r6, r6, #0x01\n"
        "msr   CONTROL, r6\n"
        "isb\n"
        "push  {r0-r3, lr}\n"
        "mov   r0, r5\n"
        "bl    os_mpu_configure_task\n"
        "pop   {r0-r3, lr}\n"
        "b     9f\n"
        "8:\n"
        "mrs   r6, CONTROL\n"
        "bic   r6, r6, #0x01\n"
        "msr   CONTROL, r6\n"
        "isb\n"
        "9:\n"
#endif

        "ldmia r0!, {r4-r11}\n"
        "msr psp, r0\n"
        "isb\n"
        "bx lr\n"
        ".ltorg\n"
    );
}
#endif /* OS_HAS_FPU_HW */
