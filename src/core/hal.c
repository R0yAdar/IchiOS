#include "hal.h"

idt_descriptor idt_create_descriptor(void* handler) {
    idt_descriptor desc;

    desc.segment_selector = 0x8;

    desc.flags = 0x8E;

    desc.ist = 0;

    desc.offset_low16 = (uint64_t)handler & 0xffff;
    desc.offset_upper16 = ((uint64_t)handler >> 16) & 0xffff;
    desc.offset_upper32 = (uint32_t)((uint64_t)handler >> 32);

    desc.reserved = 0;

    return desc;
}

gdt_desc gdt_create_descriptor(uint32_t base, uint32_t limit, uint16_t flag)
{
    gdt_desc descriptor = 0;

    // Create the high 32 bit segment
    descriptor  =  limit       & 0x000F0000;         // set limit bits 19:16
    descriptor |= (flag <<  8) & 0x00F0FF00;         // set type, p, dpl, s, g, d/b, l and avl fields
    descriptor |= (base >> 16) & 0x000000FF;         // set base bits 23:16
    descriptor |=  base        & 0xFF000000;         // set base bits 31:24
 
    // Shift by 32 to allow for low part of segment
    descriptor <<= 32;
 
    // Create the low 32 bit segment
    descriptor |= base  << 16;                       // set base bits 15:0
    descriptor |= limit  & 0x0000FFFF;               // set limit bits 15:0
    
    return descriptor;
}