#include "../cpu/ports.h"
#include "x86_pit.h"
#include "x86_pic.h"
#include "vga.h"

long long current_time = 0;
long long current_char = 0;

void init_pit() {
    // add code to setup the timer...
    short frequency = 18;

    short divisor = PIT_OSCILLATOR_SIGNAL_HZ / frequency;

    port_outb(PIT_COMMAND_PORT, 0x36);
    port_outb(PIT_CHANNEL0_PORT, (divisor & 0xFF));
    port_outb(PIT_CHANNEL0_PORT, (divisor >> 8));
}

void int_to_string(char* array, long long value){
    int pos = 0;

    while (value > 0)
    {
        array[pos++] = '0' + value % 10;
        value /= 10;
    }

    if (pos == 0){
        array[pos] = 0;
    }
}


void pit_irq_handler() {
    char string_int_buffer[30] = {};

    // int_to_string(string_int_buffer, ++current_time);

	char pit_message[] = "Nani ";

    pit_message[5] = current_char++;

	vga_text_input input  = {0, 3, pit_message, 0x09};
	vga_put(&input);
    // input.text = string_int_buffer;
    // input.x += sizeof(pit_message) + 6;
    // vga_put(&input);
    
    port_outb(PIC1_COMMAND_PORT, 0x20); // Eoi
}