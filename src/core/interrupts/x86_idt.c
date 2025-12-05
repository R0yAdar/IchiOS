#include "stdint.h"
#include "../hal.h"
#include "x86_idt.h"
#include "vga.h"
#include "print.h"
#include "assembly.h"

static idt_descriptor _idt[256];

static idtr _idtr;

void syscall(uint64_t id, void* ptr)
{
    interrupt80(id, ptr);
}

void general_exception_handler(uint64_t exception_no, void* ptr) {
    vga_text_input input;

    input.x = 10;
    input.y = 0;

    input.text = exception_messages[exception_no];

    input.color = 0x1;

    vga_put(&input);
}

void sysCallC(uint64_t syscall_no, void* ptr) {
	const char msg[] = "DEFAULT INTERRUPT HANDLER!!";

    vga_text_input input;

    input.x = 10;
    input.y = 10;

    input.text = msg;

    input.color = 0x38;

    vga_put(&input);
}

void i86_install_ir(uint8_t index, idt_descriptor descriptor){
    _idt[index] = descriptor;
}

int init_idt(){
    _idtr.limit = sizeof(idt_descriptor) * 256 -1;
    _idtr.base = (uint64_t)&_idt[0];

    i86_install_ir(0,  idt_create_descriptor(isr0_handler));
    i86_install_ir(1,  idt_create_descriptor(isr1_handler));
    i86_install_ir(2,  idt_create_descriptor(isr2_handler));
    i86_install_ir(3,  idt_create_descriptor(isr3_handler));
    i86_install_ir(4,  idt_create_descriptor(isr4_handler));
    i86_install_ir(5,  idt_create_descriptor(isr5_handler));
    i86_install_ir(6,  idt_create_descriptor(isr6_handler));
    i86_install_ir(7,  idt_create_descriptor(isr7_handler));
    i86_install_ir(8,  idt_create_descriptor(isr8_handler));
    i86_install_ir(9,  idt_create_descriptor(isr9_handler));
    i86_install_ir(10, idt_create_descriptor(isr10_handler));
    i86_install_ir(11, idt_create_descriptor(isr11_handler));
    i86_install_ir(12, idt_create_descriptor(isr12_handler));
    i86_install_ir(13, idt_create_descriptor(isr13_handler));
    i86_install_ir(14, idt_create_descriptor(isr14_handler));
    i86_install_ir(15, idt_create_descriptor(isr15_handler));
    i86_install_ir(16, idt_create_descriptor(isr16_handler));
    i86_install_ir(17, idt_create_descriptor(isr17_handler));
    i86_install_ir(18, idt_create_descriptor(isr18_handler));
    i86_install_ir(19, idt_create_descriptor(isr19_handler));
    i86_install_ir(20, idt_create_descriptor(isr20_handler));
    i86_install_ir(21, idt_create_descriptor(isr21_handler));
    i86_install_ir(22, idt_create_descriptor(isr22_handler));
    i86_install_ir(23, idt_create_descriptor(isr23_handler));
    i86_install_ir(24, idt_create_descriptor(isr24_handler));
    i86_install_ir(25, idt_create_descriptor(isr25_handler));
    i86_install_ir(26, idt_create_descriptor(isr26_handler));
    i86_install_ir(27, idt_create_descriptor(isr27_handler));
    i86_install_ir(28, idt_create_descriptor(isr28_handler));
    i86_install_ir(29, idt_create_descriptor(isr29_handler));
    i86_install_ir(30, idt_create_descriptor(isr30_handler));
    i86_install_ir(31, idt_create_descriptor(isr31_handler));
    
    // PIT
    i86_install_ir(32, idt_create_descriptor(isr32_handler));
    i86_install_ir(33, idt_create_descriptor(isr33_handler));

    for (int i = 34; i < 256; i++){
        i86_install_ir(i, idt_create_descriptor(isr80_handler));
    }

    load_idtr(_idtr);

    return 0;    
}

