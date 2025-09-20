#ifndef GDT_H
#define GDT_H

#include "types.h"

void init_gdt(void* tss, uint16_t tss_size);

#endif