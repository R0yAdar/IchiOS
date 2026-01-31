#include "stdint.h"
#include "idt.h"
#include "pit.h"
#include "assembly.h"
#include "serial.h"
#include "../hal.h"

char *EXCEPTION_CODE_TO_NAME[] = {
    "Division by Zero",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",

    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bat TSS",
    "Segment not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",

    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",

    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"};

static idt_descriptor _idt[IDT_ENTRY_COUNT];

static idtr _idtr;

void opaque_handler(uint64_t id, void *ptr)
{
    qemu_logf("Opaque handler called (id=%d, ptr=0x%x)!!!", id, ptr);
}

void general_exception_handler(uint64_t exception_no, void *ptr)
{
    uint64_t faulting_address = read_cr2();

    qemu_logf("%s CR2: %x", EXCEPTION_CODE_TO_NAME[exception_no], faulting_address);

    while (1)
    {
        hlt();
    }
    (void)ptr;
}

void idt_install_handler(uint8_t index, idt_descriptor descriptor)
{
    _idt[index] = descriptor;
}

int idt_init()
{
    _idtr.limit = sizeof(idt_descriptor) * IDT_ENTRY_COUNT - 1;
    _idtr.base = (uint64_t)&_idt[0];

    idt_install_handler(0, idt_create_descriptor(isr0_handler));
    idt_install_handler(1, idt_create_descriptor(isr1_handler));
    idt_install_handler(2, idt_create_descriptor(isr2_handler));
    idt_install_handler(3, idt_create_descriptor(isr3_handler));
    idt_install_handler(4, idt_create_descriptor(isr4_handler));
    idt_install_handler(5, idt_create_descriptor(isr5_handler));
    idt_install_handler(6, idt_create_descriptor(isr6_handler));
    idt_install_handler(7, idt_create_descriptor(isr7_handler));
    idt_install_handler(8, idt_create_descriptor(isr8_handler));
    idt_install_handler(9, idt_create_descriptor(isr9_handler));
    idt_install_handler(10, idt_create_descriptor(isr10_handler));
    idt_install_handler(11, idt_create_descriptor(isr11_handler));
    idt_install_handler(12, idt_create_descriptor(isr12_handler));
    idt_install_handler(13, idt_create_descriptor(isr13_handler));
    idt_install_handler(14, idt_create_descriptor(isr14_handler));
    idt_install_handler(15, idt_create_descriptor(isr15_handler));
    idt_install_handler(16, idt_create_descriptor(isr16_handler));
    idt_install_handler(17, idt_create_descriptor(isr17_handler));
    idt_install_handler(18, idt_create_descriptor(isr18_handler));
    idt_install_handler(19, idt_create_descriptor(isr19_handler));
    idt_install_handler(20, idt_create_descriptor(isr20_handler));
    idt_install_handler(21, idt_create_descriptor(isr21_handler));
    idt_install_handler(22, idt_create_descriptor(isr22_handler));
    idt_install_handler(23, idt_create_descriptor(isr23_handler));
    idt_install_handler(24, idt_create_descriptor(isr24_handler));
    idt_install_handler(25, idt_create_descriptor(isr25_handler));
    idt_install_handler(26, idt_create_descriptor(isr26_handler));
    idt_install_handler(27, idt_create_descriptor(isr27_handler));
    idt_install_handler(28, idt_create_descriptor(isr28_handler));
    idt_install_handler(29, idt_create_descriptor(isr29_handler));
    idt_install_handler(30, idt_create_descriptor(isr30_handler));
    idt_install_handler(31, idt_create_descriptor(isr31_handler));

    // PIT
    idt_install_handler(32, idt_create_descriptor(isr32_handler));
    idt_install_handler(33, idt_create_descriptor(isr33_handler));

    // Rest...
    for (int i = 34; i < 256; i++)
    {
        if (i == 0x80)
        {
            idt_install_handler(0x80, idt_create_userland_descriptor(isr80_handler));
        }
        else
        {
            idt_install_handler(i, idt_create_descriptor(opaque_handler));
        }
    }

    idtr_load(_idtr);

    return 0;
}
