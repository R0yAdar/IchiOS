#ifndef VMEM_H
#define VMEM_H

#include "types.h"
#include "multiboot.h"

#define HIGHER_HALF_KERNEL_OFFSET 0xffffffff80000000
#define ENTRIES_PER_TABLE 512
#define RAM_DIRECT_MAPPING_OFFSET 0xffff800000000000
#define RAM_MMIO_MAPPING_OFFSET 0xffff900000000000
#define ALIGN_4KB(ptr) ((void*)((uint64_t)(ptr) & (~0xFFF)))

typedef struct pagetable_context pagetable_context;

void* vphys_address(void* phys);

ERROR_CODE init_vmem();

void* vmem_allocate_page();

void vmem_free_page();

void* vmem_lookup_paddress(void* virt);

void vmem_map_page(void* phys, void* virt);

void* kpage_alloc(size_t page_count);

void* kpage_alloc_dma(size_t page_count, void** out_phys_address);

void kpage_free_dma(size_t page_count, void* vaddr, void* phys);

void kpage_free(void* vaddr, size_t page_count);

void* kmalloc(size_t len);

void kfree(void* vaddr);

void* vmm_map_mmio_region(pagetable_context* ctx, void* phys_start, void* phys_end);

pagetable_context* vmm_get_global_context();

pagetable_context* vmm_create_userspace_context();

size_t vmm_allocate_umm(pagetable_context* ctx, uint64_t vaddress, size_t len);

void vmm_apply_pagetable(pagetable_context* ctx);

#endif