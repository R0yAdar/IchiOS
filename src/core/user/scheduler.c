#include "serial.h"
#include "assembly.h"

#pragma pack(push, 1)

typedef struct
{
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rax;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} stack_layout;

#pragma pack(pop)

stack_layout previous;
uint64_t count = 0;

uint64_t scheduler_check(uint64_t rsp)
{
    cli();
    volatile stack_layout *stack = (stack_layout *)rsp;

    if (stack->rip < 0x80000000)
    {
        ++count;

        if (count % 10 == 0)
        {
            qemu_log("Here we go!");
            previous = *stack;
            return 1;
        }
    }

    //qemu_logf("Scheduler check done %x", rsp);

    sti();

    return 0;
}

__attribute__((naked, noreturn)) void scheduler_switch()
{
    qemu_log("Scheduler switch");
    asm volatile(
        "mov %0, %%r11\n\t"
        "mov %1, %%r10\n\t"
        "mov %2, %%r9\n\t"
        "mov %3, %%r8\n\t"
        "mov %4, %%rdi\n\t"
        "mov %5, %%rsi\n\t"
        "mov %6, %%rdx\n\t"
        "mov %7, %%rcx\n\t"
        "mov %8, %%rax\n\t"
        :
        : "g"(previous.r11), "g"(previous.r10), "g"(previous.r9),
          "g"(previous.r8), "g"(previous.rdi), "g"(previous.rsi),
          "g"(previous.rdx), "g"(previous.rcx), "g"(previous.rax)
        : "r11", "r10", "r9", "r8", "rdi", "rsi", "rdx", "rcx", "rax");

    qemu_logf("Jumping to userland at %x, stack at %x", previous.rip, previous.rsp);
    jump_to_userland(previous.rsp, previous.rip);
}