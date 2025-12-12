#include "assembly.h"
#include "pit.h"
#include "pic.h"
#include "vga.h"
#include "str.h"
#include "stdint.h"


unsigned volatile long long current_time = 0;
unsigned volatile long long current_time_ms = 0;
unsigned volatile long long current_char = 0;

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

    current_time_ms += 1000 / 20;

    input.x += 2;
    input.text = int_to_str((++current_time) / 20);
    vga_put(&input);
    if (current_time % 20 == 0) {
        qemu_log("tick");
    }
}

void sleep(uint64_t ms) {
    sti();
    uint64_t end_time = current_time_ms + ms;

    while (current_time_ms < end_time) {
        hlt();
    }
    cli();
}