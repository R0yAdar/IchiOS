#include "gdt.h"
#include "print.h"
#include "assembly.h"
#include "../hal.h"

gdt_table table;
gdt_ptr _gdt_ptr;


void init_gdt() {
    table.null = gdt_create_descriptor(0, 0, 0);
    table.code_pl0 = gdt_create_descriptor(0, 0x0000FFFF, GDT_CODE_PL0);
    table.data_pl0 = gdt_create_descriptor(0, 0x0000FFFF, GDT_DATA_PL0);
    table.code_pl3 = gdt_create_descriptor(0, 0x0000FFFF, GDT_CODE_PL3);
    table.data_pl3 = gdt_create_descriptor(0, 0x0000FFFF, GDT_DATA_PL3);

    _gdt_ptr.limit = sizeof(gdt_desc) * 5 - 1;
    _gdt_ptr.base = &table;
    
    load_gdtr(&_gdt_ptr);

    extern void reloadSegments();

    reloadSegments();
}
