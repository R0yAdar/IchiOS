#ifndef SERIAL_H
#define SERIAL_H

#define PORT_COM1 0x3f8
#define PORT_COM1_DATA 0x3f8
#define PORT_COM1_STATUS 0x3fd

void qemu_log(const char* s);

void serial_init();

#endif