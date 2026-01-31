#include "gdt.h"
#include "assembly.h"
#include "../hal.h"

extern void gdtReloadSegments();

gdt_table _table;
gdt_ptr _gdt_ptr;

void gdt_init(void* tss, uint16_t tss_size) {
    _table.null = gdt_create_descriptor(0, 0, 0);
    _table.code_pl0 = gdt_create_descriptor(0, 0x0000FFFF, GDT_CODE_PL0);
    _table.data_pl0 = gdt_create_descriptor(0, 0x0000FFFF, GDT_DATA_PL0);
    _table.code_pl3 = gdt_create_descriptor(0, 0x0000FFFF, GDT_CODE_PL3);
    _table.data_pl3 = gdt_create_descriptor(0, 0x0000FFFF, GDT_DATA_PL3);
    _table.tss = gdt_create_tss_descriptor((uint64_t)tss, tss_size);

    _gdt_ptr.limit = sizeof(gdt_table) - 1;
    _gdt_ptr.base = (uint64_t)&_table;
    
    gdtr_load(&_gdt_ptr);

    gdtReloadSegments();

    ltr();
}
