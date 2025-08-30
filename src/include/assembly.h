#ifndef ASSEMBLY_H
#define ASSEMBLY_H

#include "stdint.h"

#pragma pack (push, 1)

typedef struct idtr_t
{
    uint16_t limit;
    uint64_t base;
} idtr;

#pragma pack (pop)

uint64_t read_cr3();

void write_cr3(uint64_t value);

void cli();

void sti();

void hlt();

void set_rsp(void* stack_top);

void load_gdtr(void* gdtr);

uint8_t port_inb(uint8_t port);

void port_outb(uint8_t port, uint8_t data);

void load_idtr(idtr idtr);

void interrupt80(uint64_t arg1, void* arg2);

void flush_tlb(uint64_t address);

#endif