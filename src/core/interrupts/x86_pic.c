#include "x86_pic.h"
#include "assembly.h"
#include "stdint.h"


void remap_pic() {
    // Restart PICs
    port_outb(PIC1_COMMAND_PORT, 0x11);
    port_outb(PIC2_COMMAND_PORT, 0x11);

    // Set new IDT offsets
    port_outb(PIC1_DATA_PORT, 0x20); // 32...
    port_outb(PIC2_DATA_PORT, 0x28); // 40...

    // Setup cascading
    port_outb(PIC1_DATA_PORT, 0x04);
    port_outb(PIC2_DATA_PORT, 0x02);

    // Finish
    port_outb(PIC1_DATA_PORT, 0x01);
    port_outb(PIC2_DATA_PORT, 0x01);
}

void init_pic() {
    remap_pic();

    uint8_t pic1_enabled_irqs = PIC1_SYSTEM_TIMER | PIC1_KEYBOARD_CONTROLLER;
    port_outb(PIC1_DATA_PORT, (~pic1_enabled_irqs));

    uint8_t pic2_enabled_irqs = 0;
    port_outb(PIC2_DATA_PORT, pic2_enabled_irqs);
}
