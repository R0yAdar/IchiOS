#include "hal.h"

idt_descriptor _idt_create_descriptor(void (*handler)(), uint8_t dpl)
{
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

idt_descriptor idt_create_descriptor(void (*handler)())
{
    return _idt_create_descriptor(handler, 0);
}

idt_descriptor idt_create_userland_descriptor(void (*handler)())
{
    return _idt_create_descriptor(handler, 3);
}

gdt_desc gdt_create_descriptor(uint32_t base, uint32_t limit, uint16_t flag)
{
    gdt_desc descriptor = 0;

    descriptor = limit & 0x000F0000;
    descriptor |= (flag << 8) & 0x00F0FF00;
    descriptor |= (base >> 16) & 0x000000FF;
    descriptor |= base & 0xFF000000;

    descriptor <<= 32;

    descriptor |= base << 16;
    descriptor |= limit & 0x0000FFFF;

    return descriptor;
}

gdt_tss_descriptor gdt_create_tss_descriptor(uint64_t tss_address, uint16_t tss_size)
{
    gdt_tss_descriptor descriptor = {0};

    uint32_t limit = tss_size - 1;

    descriptor.limit_low = limit & 0xFFFF;
    descriptor.limit_high = (limit >> 16) & 0xF;

    descriptor.base_low = (uint16_t)(tss_address & 0xFFFF);
    descriptor.base_mid = (uint8_t)((tss_address >> 16) & 0xFF);
    descriptor.base_high = (uint8_t)((tss_address >> 24) & 0xFF);
    descriptor.base_upper = (uint32_t)(tss_address >> 32);

    descriptor.type = 0x9;
    descriptor.s = 0;
    descriptor.dpl = 0;
    descriptor.p = 1;

    descriptor.g = 0;
    descriptor.db = 0;
    descriptor.l = 0;
    descriptor.avl = 0;

    descriptor.reserved = 0;

    return descriptor;
}

tss create_tss_segment(void *kernel_stack_top)
{
    tss tss = {0};
    tss.rsp0 = (uint64_t)kernel_stack_top;
    tss.iopb_offset = sizeof(tss);

    return tss;
}