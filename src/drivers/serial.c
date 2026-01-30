#include "serial.h"
#include "assembly.h"
#include "str.h"
#include "stdarg.h"
#include "cstring.h"

int serial_received()
{
   return port_inb(PORT_COM1_STATUS) & 1;
}

char read_serial()
{
   while (serial_received() == 0)
      ;
   return port_inb(PORT_COM1_DATA);
}

int is_transmit_empty()
{
   return port_inb(PORT_COM1_STATUS) & 0x20;
}

void write_serial(char c)
{
   while (is_transmit_empty() == 0)
      ;
   port_outb(PORT_COM1_DATA, c);
}

void qemu_logf(const char *format, ...)
{
   va_list args;
   va_start(args, format);

   char buffer[1024];

   vsprintf(buffer, format, args);

   qemu_log(buffer);

   va_end(args);
}

void qemu_dump(void *buffer, uint64_t size)
{
   char out_buffer[3];
   out_buffer[2] = ' ';

   for (uint64_t i = 0; i < size; i++)
   {
      if (i != 0 && i % 16 == 0)
         write_serial('\n');
      if (number_as_string(((uint8_t *)buffer)[i], out_buffer, 16) == 1)
      {
         out_buffer[1] = out_buffer[0];
         out_buffer[0] = '0';
      };

      write_serial(out_buffer[0]);
      write_serial(out_buffer[1]);
      write_serial(out_buffer[2]);
   }
   write_serial('\n');
}

void qemu_log(const char *str)
{
   while (*str != '\0')
   {
      write_serial(*str);
      ++str;
   }

   write_serial('\n');
}

void qemu_puts(char* str)
{
   while (*str != '\0')
   {
      write_serial(*str);
      ++str;
   }
}

void serial_init()
{
   port_outb(PORT_COM1 + 1, 0x00);
   port_outb(PORT_COM1 + 3, 0x80);
   port_outb(PORT_COM1 + 0, 0x03);
   port_outb(PORT_COM1 + 1, 0x00);
   port_outb(PORT_COM1 + 3, 0x03);
   port_outb(PORT_COM1 + 2, 0xC7);
   port_outb(PORT_COM1 + 4, 0x0B);

   qemu_log("Hello QEMU, Ichi started!");
}