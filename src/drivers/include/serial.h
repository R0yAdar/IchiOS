#ifndef SERIAL_H
#define SERIAL_H

#include "types.h"

#define PORT_COM1 0x3f8
#define PORT_COM1_DATA 0x3f8
#define PORT_COM1_STATUS 0x3fd

void qemu_log(const char* s);

void qemu_log_int(uint64_t i);

void serial_init();

#endif