#include "assembly.h"

uint64_t read_cr3()
{
    uint64_t value;

    asm volatile(
        "mov %%cr3, %0"
        : "=r"(value));

    return value;
}

void write_cr3(uint64_t value)
{
    asm volatile(
        "mov %0, %%cr3"
        : : "r"(value)
        : "memory");
}

void cli()
{
    asm volatile("cli" ::: "memory");
}

void sti()
{
    asm volatile("sti" ::: "memory");
}

void hlt()
{
    asm volatile("hlt" ::: "memory");
}

void switch_stack(void *new_stack, void (*func)(void))
{
    asm volatile(
        "mov %0, %%rsp\n"
        "jmp *%1\n"
        :
        : "r"(new_stack), "r"(func)
        : "memory");
}

void load_gdtr(void *gdtr)
{
    asm volatile(
        "lgdt (%0)"
        :
        : "r"(gdtr)
        : "memory");
}

void load_task_register()
{
    asm volatile("ltr %%ax" ::"a"(0x28));
}

uint8_t port_inb(uint16_t port)
{
    uint8_t result;
    asm volatile("inb %1, %0"
                 : "=a"(result)
                 : "Nd"(port));
    return result;
}

void port_outb(uint16_t port, uint8_t data)
{
    asm volatile("outb %0, %1"
                 :
                 : "a"(data), "Nd"(port));
}

void port_outl(uint16_t port, uint32_t val)
{
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

uint32_t port_inl(uint16_t port)
{
    uint32_t ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void idtr_load(idtr idtr)
{
    asm volatile(
        "lidt %0"
        :
        : "m"(idtr));
}

void interrupt80(uint64_t arg1, void *arg2)
{
    asm volatile("int $0x80" ::"a"(arg1), "c"(arg2) : "memory");
}

void flush_tlb(uint64_t address)
{
    asm volatile("invlpg (%0)" ::"r"(address) : "memory");
}

void flush_tlb_all()
{
    void *cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    asm volatile("mov %0, %%cr3" ::"r"(cr3));
}

__attribute__((naked, noreturn)) void jump_to_userland(uint64_t stack_addr, uint64_t code_addr)
{
    asm volatile(
        "mov $0x23, %%ax \n\t"
        "mov %%ax, %%ds  \n\t"
        "mov %%ax, %%es  \n\t"
        "mov %%ax, %%fs  \n\t"
        "mov %%ax, %%gs  \n\t"
        "push $0x23 \n\t"
        "push %0    \n\t"
        "push $0x202 \n\t"
        "push $0x1B  \n\t"
        "push %1     \n\t"
        "iretq"
        :
        : "r"(stack_addr), "r"(code_addr)
        : "ax", "memory");
}

uint64_t read_cr2()
{
    uint64_t cr2;

    __asm__ volatile(
        "mov %%cr2, %0"
        : "=r"(cr2)
        :
        :);

    return cr2;
}

__attribute__((naked, noreturn)) void iretq()
{
    asm volatile(
        "iretq"
        :
        :   
        : "memory");
}