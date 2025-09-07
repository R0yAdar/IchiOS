#ifndef VMEM_H
#define VMEM_H

#include "types.h"
#include "multiboot.h"

ERROR_CODE init_vmem();

void*   vmem_allocate_page();

void    vmem_free_page();

void*   vmem_lookup_paddress(void* virt);

void    vmem_map_page(void* phys, void* virt);

void*   kmalloc(size_t len);

void    kfree(void* vaddr);

#endif