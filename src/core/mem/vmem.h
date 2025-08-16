#ifndef VMEM_H
#define VMEM_H

#include "err.h"
#include "multiboot.h"

ERROR_CODE init_vmem();

void*   vmem_allocate_page();

void    vmem_free_page();

void*   vmem_lookup_vaddress(void* phys);

void    vmem_map_page(void* phys, void* virt);

void    vmem_flush_tlb_entry();

#endif