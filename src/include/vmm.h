#ifndef VMEM_H
#define VMEM_H

#include "types.h"
#include "multiboot.h"

#define VMM_HIGHER_HALF_KERNEL_OFFSET 0xffffffff80000000
#define VMM_PTABLE_ENTRY_COUNT 512
#define VMM_RAM_DIRECT_MAPPING_OFFSET 0xffff800000000000
#define VMM_RAM_MMIO_MAPPING_OFFSET 0xffff900000000000
#define VMM_ALIGN_4KB(ptr) ((void*)(((uint64_t)(ptr) + 4095) & (~0xFFF)))

typedef struct pagetable_context pagetable_context;

ERROR_CODE vmm_init();

void* kpage_alloc(size_t page_count);

void* kpage_alloc_dma(size_t page_count, void** out_phys_address);

void kpage_free_dma(size_t page_count, void* vaddr, void* phys);

void kpage_free(void* vaddr, size_t page_count);

void* kmalloc(size_t len);

void kfree(void* vaddr);

void* vmm_get_vaddr(void* phys);

void* vmm_map_mmio_region(pagetable_context* ctx, void* phys_start, void* phys_end);

BOOL vmm_is_mmio(void* virt, void* phys);

void vmm_free_page_entry(pagetable_context* ctx, void* vaddr);

pagetable_context* vmm_get_global_context();

pagetable_context* vmm_create_userspace_context();

void vmm_destroy_userspace_context(pagetable_context* ctx);

void* vmm_allocate_umm(pagetable_context* ctx, uint64_t vaddress, size_t len);

void vmm_apply_pagetable(pagetable_context* ctx);

#endif