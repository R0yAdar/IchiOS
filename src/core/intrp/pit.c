#include "assembly.h"
#include "pit.h"
#include "pic.h"
#include "vga.h"
#include "str.h"
#include "serial.h"
#include "stdint.h"

unsigned volatile long long current_time = 0;
unsigned volatile long long current_time_ms = 0;
unsigned volatile long long current_char = 0;

void pit_init()
{
    uint32_t frequency = 20;

    uint16_t divisor = (PIT_OSCILLATOR_SIGNAL_HZ / frequency);

    port_outb(PIT_COMMAND_PORT, 0x36);
    port_outb(PIT_CHANNEL0_PORT, (divisor & 0xFF));
    port_outb(PIT_CHANNEL0_PORT, (divisor >> 8));
}

void pit_irq_handler()
{
    current_time_ms += 1000 / 20;
}

uint64_t pit_get_current_time_ms()
{
    return current_time_ms;
}

void sleep(uint64_t ms)
{
    uint64_t end_time = current_time_ms + ms;

    while (current_time_ms < end_time)
    {
        hlt();
    }
}