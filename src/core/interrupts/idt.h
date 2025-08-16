#ifndef IDT_H
#define IDT_H

#include "stdint.h"

int init_idt();

void syscall(uint64_t id, void* ptr);

#endif