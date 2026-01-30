#ifndef SERIAL_H
#define SERIAL_H

#include "types.h"

#define PORT_COM1 0x3f8
#define PORT_COM1_DATA 0x3f8
#define PORT_COM1_STATUS 0x3fd

void qemu_logf(const char* format, ...);

void qemu_log(const char* s);

void qemu_dump(void* buffer, uint64_t size);

void qemu_putc(char c);

void serial_init();

#endif