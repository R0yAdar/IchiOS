#include "serial.h"
#include "assembly.h"
#include "str.h"


int serial_received() {
   return port_inb(PORT_COM1_STATUS) & 1;
}

char read_serial() {
   while (serial_received() == 0);
   return port_inb(PORT_COM1_DATA);
}

int is_transmit_empty() {
   return port_inb(PORT_COM1_STATUS) & 0x20;
}

void write_serial(char c) {
   while (is_transmit_empty() == 0);
   port_outb(PORT_COM1_DATA, c);
}

void qemu_log(const char* str) {
    while (*str != '\0')
    {
        write_serial(*str);
        ++str;
    }
    
    write_serial('\n');
}

void qemu_log_int(uint64_t i)  {
   qemu_log(int_to_str(i));
}

void serial_init() {
   port_outb(PORT_COM1 + 1, 0x00);
   port_outb(PORT_COM1 + 3, 0x80);
   port_outb(PORT_COM1 + 0, 0x03);
   port_outb(PORT_COM1 + 1, 0x00);
   port_outb(PORT_COM1 + 3, 0x03);
   port_outb(PORT_COM1 + 2, 0xC7);
   port_outb(PORT_COM1 + 4, 0x0B);

   qemu_log("Hello QEMU, Ichi started!");
}