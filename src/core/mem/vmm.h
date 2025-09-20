#ifndef VMEM_H
#define VMEM_H

#include "types.h"
#include "multiboot.h"

#define HIGHER_HALF_KERNEL_OFFSET 0xffffffff80000000

ERROR_CODE init_vmem();

void*   vmem_allocate_page();

void    vmem_free_page();

void*   vmem_lookup_paddress(void* virt);

void    vmem_map_page(void* phys, void* virt);

void*   kpage_alloc(size_t page_count);

void    kpage_free(void* vaddr, size_t page_count);

void*   kmalloc(size_t len);

void    kfree(void* vaddr);

size_t allocate_umm(uint64_t vaddr_start, size_t len);

#endif