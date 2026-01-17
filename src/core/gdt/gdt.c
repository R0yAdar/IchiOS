#include "gdt.h"
#include "assembly.h"
#include "../hal.h"

gdt_table table;
gdt_ptr _gdt_ptr;


void init_gdt(void* tss, uint16_t tss_size) {
    table.null = gdt_create_descriptor(0, 0, 0);
    table.code_pl0 = gdt_create_descriptor(0, 0x0000FFFF, GDT_CODE_PL0);
    table.data_pl0 = gdt_create_descriptor(0, 0x0000FFFF, GDT_DATA_PL0);
    table.code_pl3 = gdt_create_descriptor(0, 0x0000FFFF, GDT_CODE_PL3);
    table.data_pl3 = gdt_create_descriptor(0, 0x0000FFFF, GDT_DATA_PL3);
    table.tss = gdt_create_tss_descriptor((uint64_t)tss, tss_size);

    _gdt_ptr.limit = sizeof(gdt_table) - 1;
    _gdt_ptr.base = (uint64_t)&table;
    
    load_gdtr(&_gdt_ptr);

    extern void reloadSegments();

    reloadSegments();

    asm volatile("ltr %%ax" :: "a"(0x28));
}
