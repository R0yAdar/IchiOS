#include "vmm.h"
#include "pmm.h"
#include "cstring.h"
#include "types.h"

void* kpage_alloc(size_t page_count) {
    void* paddr = pmm_alloc_blocks(page_count);
    
    if (paddr == NULL) return NULL;
    
    void* vaddr = (void*)(RAM_DIRECT_MAPPING_OFFSET | (uint64_t)paddr);
    memset((void*)vaddr, 0, PAGE_SIZE * page_count);

    return vaddr;
}

void* kpage_alloc_dma(size_t page_count, void** out_phys_address) {
    void* paddr = pmm_alloc_blocks(page_count);

    if (paddr == NULL) return NULL;
    void* vaddr = vmm_map_mmio_region(vmm_get_global_context(), paddr, (void*)((uint64_t)paddr + PAGE_SIZE * page_count));
    memset((void*)vaddr, 0, PAGE_SIZE * page_count);

    (*out_phys_address) = paddr;

    return vaddr;
}

void kpage_free_dma(size_t page_count, void* vaddr, void* phys) {
    if (!phys || !vaddr) return;
    // you can verify that phys and vaddr match

    pmm_free_blocks(phys, page_count);

    // TODO: remove virtual mapping
}


void kpage_free(void* vaddr, size_t page_count) {
    if (vaddr == NULL) return;
    if (((uint64_t)vaddr & RAM_DIRECT_MAPPING_OFFSET) != RAM_DIRECT_MAPPING_OFFSET) return;

    pmm_free_blocks((void*)((~RAM_DIRECT_MAPPING_OFFSET) & (uint64_t)vaddr), page_count);
}
