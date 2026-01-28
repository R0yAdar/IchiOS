#ifndef GDT_H
#define GDT_H

#include "types.h"

void gdt_init(void* tss, uint16_t tss_size);

#endif