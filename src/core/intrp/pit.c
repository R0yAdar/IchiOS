#include "assembly.h"
#include "pit.h"
#include "pic.h"
#include "vga.h"
#include "str.h"
#include "serial.h"
#include "stdint.h"
#include "vmm.h"

#define PIT_MAX_SUBSCRIBERS 256

unsigned volatile long long current_time = 0;
unsigned volatile long long current_time_ms = 0;
unsigned volatile long long current_char = 0;

uint64_t *_deadlines;
pit_subscriber_callback *_subscribers;
void **_subscriber_data;

void pit_init()
{
    uint32_t frequency = 20;

    uint16_t divisor = (PIT_OSCILLATOR_SIGNAL_HZ / frequency);

    port_outb(PIT_COMMAND_PORT, 0x36);
    port_outb(PIT_CHANNEL0_PORT, (divisor & 0xFF));
    port_outb(PIT_CHANNEL0_PORT, (divisor >> 8));

    _deadlines = (uint64_t *)kmalloc(sizeof(uint64_t) * PIT_MAX_SUBSCRIBERS);
    _subscribers = (pit_subscriber_callback *)kmalloc(sizeof(pit_subscriber_callback) * PIT_MAX_SUBSCRIBERS);
    _subscriber_data = (void **)kmalloc(sizeof(void *) * PIT_MAX_SUBSCRIBERS);
}

void pit_irq_handler()
{
    current_time_ms += 1000 / 20;

    for (uint16_t i = 0; i < PIT_MAX_SUBSCRIBERS; i++)
    {
        if (_subscribers[i] && current_time_ms >= _deadlines[i])
        {
            _subscribers[i](_subscriber_data[i]);
            _subscribers[i] = NULL;
        }
    }
}

uint64_t pit_get_current_time_ms()
{
    return current_time_ms;
}

BOOL pit_subscribe(pit_subscriber_callback callback, void *data, uint32_t relative_deadline_ms)
{
    for (uint16_t i = 0; i < PIT_MAX_SUBSCRIBERS; i++)
    {
        if (!_subscribers[i])
        {
            _subscribers[i] = callback;
            _subscriber_data[i] = data;
            _deadlines[i] = current_time_ms + relative_deadline_ms;
            return TRUE;
        }
    }

    return FALSE;
}
