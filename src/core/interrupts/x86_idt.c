#include "stdint.h"
#include "../hal.h"
#include "x86_idt.h"
#include "vga.h"
#include "print.h"

#pragma pack (push, 1)

typedef struct
{
    uint16_t limit;
    uint64_t base;
} idtr;

#pragma pack (pop)

static idt_descriptor _idt[256];

static idtr _idtr;

static void idt_install() {
    asm volatile (
        "lidt %0"
        :
        : "m"(_idtr)
    );
}

void systemCall(int sysCallNo, void* ptr)
{
    asm volatile( "int $0xc0" :: "a"(sysCallNo), "c"(ptr) : "memory" );
}

void general_exception_handler(uint64_t exception_no, void* ptr) {
    vga_text_input input;

    input.x = 7;
    input.y = 7;

    input.text = exception_messages[exception_no];

    input.color = 0x37;

    vga_put(&input);
}

void sysCallC(uint64_t syscall_no, void* ptr) {
	const char msg[] = "DEFAULT INTERRUPT HANDLER!!";

    vga_text_input input;

    input.x = 10;
    input.y = 10;

    input.text = msg;

    input.color = 0x19;

    vga_put(&input);
}

void i86_install_ir(uint8_t index, idt_descriptor descriptor){
    _idt[index] = descriptor;
}

int init_idt(){
    _idtr.limit = sizeof(idt_descriptor) * 256 -1;
    _idtr.base = (uint64_t)&_idt[0];
    printxln(_idtr.base);

    i86_install_ir(0,  create_descriptor(isr0_handler));
    i86_install_ir(1,  create_descriptor(isr1_handler));
    i86_install_ir(2,  create_descriptor(isr2_handler));
    i86_install_ir(3,  create_descriptor(isr3_handler));
    i86_install_ir(4,  create_descriptor(isr4_handler));
    i86_install_ir(5,  create_descriptor(isr5_handler));
    i86_install_ir(6,  create_descriptor(isr6_handler));
    i86_install_ir(7,  create_descriptor(isr7_handler));
    i86_install_ir(8,  create_descriptor(isr8_handler));
    i86_install_ir(9,  create_descriptor(isr9_handler));
    i86_install_ir(10, create_descriptor(isr10_handler));
    i86_install_ir(11, create_descriptor(isr11_handler));
    i86_install_ir(12, create_descriptor(isr12_handler));
    i86_install_ir(13, create_descriptor(isr13_handler));
    i86_install_ir(14, create_descriptor(isr14_handler));
    i86_install_ir(15, create_descriptor(isr15_handler));
    i86_install_ir(16, create_descriptor(isr16_handler));
    i86_install_ir(17, create_descriptor(isr17_handler));
    i86_install_ir(18, create_descriptor(isr18_handler));
    i86_install_ir(19, create_descriptor(isr19_handler));
    i86_install_ir(20, create_descriptor(isr20_handler));
    i86_install_ir(21, create_descriptor(isr21_handler));
    i86_install_ir(22, create_descriptor(isr22_handler));
    i86_install_ir(23, create_descriptor(isr23_handler));
    i86_install_ir(24, create_descriptor(isr24_handler));
    i86_install_ir(25, create_descriptor(isr25_handler));
    i86_install_ir(26, create_descriptor(isr26_handler));
    i86_install_ir(27, create_descriptor(isr27_handler));
    i86_install_ir(28, create_descriptor(isr28_handler));
    i86_install_ir(29, create_descriptor(isr29_handler));
    i86_install_ir(30, create_descriptor(isr30_handler));
    i86_install_ir(31, create_descriptor(isr31_handler));
    
    // PIT
    i86_install_ir(32, create_descriptor(isr32_handler));

    for (int i = 33; i < 256; i++){
        i86_install_ir(i, create_descriptor(isr80_handler));
    }

    idt_install();
    
    return 0;    
}

