#include "assembly.h"
#include "stdint.h"
#include "pic.h"

void restart_pics() {
    port_outb(PIC1_COMMAND_PORT, 0x11);
    port_outb(PIC2_COMMAND_PORT, 0x11);
}

void relocate_idt_offsets() {
    port_outb(PIC1_DATA_PORT, PIC1_IDT_OFFSET);
    port_outb(PIC2_DATA_PORT, PIC2_IDT_OFFSET);
}

void setup_cascading() {
    port_outb(PIC1_DATA_PORT, 0x04);
    port_outb(PIC2_DATA_PORT, 0x02);
}

void finish_pic_setup() {
    port_outb(PIC1_DATA_PORT, 0x01);
    port_outb(PIC2_DATA_PORT, 0x01);
}

void remap_pic() {
    restart_pics();

    relocate_idt_offsets();

    setup_cascading();

    finish_pic_setup(); 
}

void init_pic() {
    remap_pic();

    uint8_t pic1_enabled_irqs = PIC1_SYSTEM_TIMER | PIC1_KEYBOARD_CONTROLLER;
    port_outb(PIC1_DATA_PORT, (~pic1_enabled_irqs));

    uint8_t pic2_enabled_irqs = 0;
    port_outb(PIC2_DATA_PORT, (~pic2_enabled_irqs));
}
