#include "assembly.h"
#include "x86_pit.h"
#include "x86_pic.h"
#include "vga.h"
#include "str.h"
#include "stdint.h"


unsigned long long current_time = 0;
unsigned long long current_char = 0;

void init_pit() {
    uint32_t frequency = 20;
    
    uint16_t divisor = (PIT_OSCILLATOR_SIGNAL_HZ / frequency);

    port_outb(PIT_COMMAND_PORT, 0x36);
    port_outb(PIT_CHANNEL0_PORT, (divisor & 0xFF));
    port_outb(PIT_CHANNEL0_PORT, (divisor >> 8));
}

void pit_irq_handler() {
	char pit_message[] = "Nani ";

    pit_message[5] = current_char++;

	vga_text_input input  = {VGA_COLUMNS_NUM - 20, 0, pit_message, 0x09};
	vga_put(&input);

    input.text = "  ";
    input.x += sizeof(pit_message);
	vga_put(&input);

    input.x += 2;
    input.text = int_to_str((++current_time) / 20);
    vga_put(&input);
    
    port_outb(PIC1_COMMAND_PORT, 0x20); // EOI (End of IRQ)
}