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

void switch_stack(void* new_stack, void (*func)(void));

void load_gdtr(void* gdtr);

void load_task_register();

uint8_t port_inb(uint16_t port);

void port_outb(uint16_t port, uint8_t data);

void port_outl(uint16_t port, uint32_t val);

uint32_t port_inl(uint16_t port);

void load_idtr(idtr idtr);

void interrupt80(uint64_t arg1, void* arg2);

void flush_tlb(uint64_t address);

void flush_tlb_all();

void jump_to_userland(uint64_t stack_addr, uint64_t code_addr);

uint64_t read_cr2();

void iretq();

#endif