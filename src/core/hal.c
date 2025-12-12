#include "hal.h"

idt_descriptor _idt_create_descriptor(void* handler, uint8_t dpl) {
    const uint8_t type = 0xE;

    idt_descriptor desc;

    desc.segment_selector = 0x8;

    uint8_t flags = (1 << 7) | ((dpl & 3) << 5) | (type);

    desc.flags = flags;

    desc.ist = 0;

    desc.offset_low16 = (uint64_t)handler & 0xffff;
    desc.offset_upper16 = ((uint64_t)handler >> 16) & 0xffff;
    desc.offset_upper32 = (uint32_t)((uint64_t)handler >> 32);

    desc.reserved = 0;

    return desc;
}

idt_descriptor idt_create_descriptor(void* handler) {
    return _idt_create_descriptor(handler, 0);
}

idt_descriptor idt_create_userland_descriptor(void* handler) {
    return _idt_create_descriptor(handler, 3);
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

gdt_tss_descriptor gdt_create_tss_descriptor(uint64_t tss_address, uint16_t tss_size) {
    gdt_tss_descriptor descriptor = {0};

    // Calculate the actual limit (size - 1)
    uint32_t limit = tss_size - 1; 

    // 1. Limit
    descriptor.limit_low = limit & 0xFFFF;
    descriptor.limit_high = (limit >> 16) & 0xF;

    // 2. Base Address (64 bits)
    descriptor.base_low    = (uint16_t)(tss_address & 0xFFFF);
    descriptor.base_mid    = (uint8_t)((tss_address >> 16) & 0xFF);
    descriptor.base_high   = (uint8_t)((tss_address >> 24) & 0xFF);
    descriptor.base_upper  = (uint32_t)(tss_address >> 32); // Correctly stored in the second QWORD

    // 3. Flags/Access Byte (P DPL S Type) - Standard for Available 64-bit TSS (0x89)
    descriptor.type = 0x9;  // 1001b = Available 64-bit TSS
    descriptor.s    = 0;    // 0 = System Descriptor
    descriptor.dpl  = 0;    // DPL = 0 (only kernel can load it)
    descriptor.p    = 1;    // 1 = Segment Present

    // 4. Flags (G D/B L AVL)
    descriptor.g    = 0;    // Granularity is byte (0) since limit is small
    descriptor.db   = 0;    // 0 for TSS
    descriptor.l    = 0;    // 0 for TSS
    descriptor.avl  = 0;    // Available (can be 1 if desired)

    // 5. Reserved field (must be 0)
    descriptor.reserved = 0;

    return descriptor;
}

tss create_tss_segment(void* kernel_stack_top) {
    tss tss = {0};
    tss.rsp0 = (uint64_t)kernel_stack_top;
    tss.iopb_offset = sizeof(tss);

    return tss;
}