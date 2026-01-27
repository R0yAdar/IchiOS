#ifndef X86_PIT_H
#define X86_PIT_H

#include "types.h"

/* PIT Ports */
#define PIT_CHANNEL0_PORT 0x40
#define PIT_CHANNEL1_PORT 0x41
#define PIT_CHANNEL2_PORT 0x42
#define PIT_COMMAND_PORT 0x43

/* Magic values */ 
#define PIT_OSCILLATOR_SIGNAL_HZ 1193182U

void pit_init();

uint64_t pit_get_current_time_ms();

void sleep(uint64_t ms);

#endif