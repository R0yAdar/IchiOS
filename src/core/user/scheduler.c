#include "serial.h"
#include "process.h"
#include "assembly.h"
#include "scheduler.h"
#include "vmm.h"

process_ctx *active = NULL;
process_ctx *processes[512] = {NULL};
uint16_t current_process = 0;

uint64_t ticks = 0;
BOOL is_time_to_switch = FALSE;

uint64_t scheduler_check(uint64_t rsp)
{
    cli();

    volatile stack_layout *stack = (stack_layout *)rsp;

    ++ticks;
    if (ticks % 100 == 0) {
        is_time_to_switch = TRUE;
    }

    if (is_time_to_switch && !IS_HIGHER_HALF(stack->rip))
    {
        /*
        volatile uint64_t *stack_i = (uint64_t*)rsp;
        qemu_logf("Stack layout:");
        for (size_t i = 0; i < 14; i++)
        {
            qemu_logf("    %x", stack_i[i]);
        }
        */
        
        if (active)
        {
            qemu_logf("Stopping process %x", active);
            process_stop(active, stack);
        }

        return 1;
    }

    return 0;
}

__attribute__((naked, noreturn)) void scheduler_switch()
{
    is_time_to_switch = FALSE;

    while (!processes[++current_process])
    {
        if (current_process == 511) current_process = 0;
    }

    active = processes[current_process];

    process_resume(active);
}

void scheduler_add_process(process_ctx *ctx)
{
    uint16_t i = 0;
    while (processes[++i]);
    processes[i] = ctx;
}

__attribute__((naked, noreturn)) void scheduler_transfer_ctrl() {
    scheduler_switch();
}