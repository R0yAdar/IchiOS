#include "serial.h"
#include "process.h"
#include "assembly.h"
#include "scheduler.h"
#include "../intrp/pit.h"
#include "vmm.h"
#include "mutex.h"

#define SCHEDULER_TIME_PER_PROCESS_MS 50
#define SCHEDULER_PROCESS_COUNT 512

/// Scheduler verdicts, in idt.asm there are interceptors that use these to determine if the scheduler should switch.
#define SCHEDULER_SHOULD_SWITCH_VERDICT 1
#define SCHEDULER_NO_SWITCH_VERDICT 0

process_ctx *_active = NULL;
process_ctx **_processes;
uint16_t _current_process = 0;
uint64_t _current_deadline = 0;

spinlock_t _lock = SPINLOCK_INIT;

BOOL should_switch = FALSE;

BOOL scheduler_init()
{
    _processes = (process_ctx **)kmalloc(sizeof(process_ctx *) * SCHEDULER_PROCESS_COUNT);
    if (!_processes)
        return FALSE;
    return TRUE;
}

uint64_t scheduler_check(uint64_t rsp)
{
    spin_lock(&_lock);
    volatile stack_layout *stack = (stack_layout *)rsp;

    if (_current_deadline < pit_get_current_time_ms() || (_active && process_get_state(_active) != PROCESS_READY))
    {
        should_switch = TRUE;
    }

    if (should_switch && !IS_HIGHER_HALF(stack->rip))
    {
        if (_active)
        {
            process_stop(_active, stack);
        }

        return SCHEDULER_SHOULD_SWITCH_VERDICT;
    }

    spin_unlock(&_lock);
    return SCHEDULER_NO_SWITCH_VERDICT;
}

__attribute__((naked, noreturn)) void scheduler_switch()
{
    should_switch = FALSE;
    _current_deadline = pit_get_current_time_ms() + SCHEDULER_TIME_PER_PROCESS_MS;
    uint32_t search_count;

    while (
        !_processes[++_current_process] ||
        ((process_get_state(_processes[_current_process]) != PROCESS_READY &&
          (process_get_state(_processes[_current_process]) != PROCESS_IDLE || search_count < SCHEDULER_PROCESS_COUNT))))
    {
        ++search_count;
        if (_current_process == (SCHEDULER_PROCESS_COUNT - 1))
        {
            _current_process = 0;
        }
    }

    _active = _processes[_current_process];

    spin_unlock(&_lock);
    process_resume(_active);
}

process_ctx *scheduler_get_active()
{
    return _active;
}

void scheduler_add_process(process_ctx *ctx)
{
    uint16_t free_cell_index = 0;
    while (_processes[++free_cell_index])
        ;
    _processes[free_cell_index] = ctx;
}

__attribute__((naked, noreturn)) void scheduler_transfer_ctrl()
{
    scheduler_switch();
}