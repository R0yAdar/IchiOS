#include "vmm.h"
#include "pmm.h"
#include "pte.h"
#include "print.h"
#include "cstring.h"
#include "assembly.h"
#include "serial.h"

/// VMM INTERNALS

typedef uint16_t ptable_index_t;

typedef enum {
    PAGE_MAPPING_DEFAULT = 0,
    PAGE_MAPPING_USERSPACE = 1,
    PAGE_MAPPING_HUGE_PAGE = 2,
    PAGE_MAPPING_NON_CACHEABLE = 4,
} PAGE_MAPPING_OPTIONS;

typedef enum {
    PTABLE_LVL_PML4 = 4,
    PTABLE_LVL_PDPT = 3,
    PTABLE_LVL_PDE = 2,
    PTABLE_LVL_PTE = 1
} PTABLE_LVL;

struct pagetable_context
{
    pte_t* root_table;
};

typedef struct {
    BOOL is_post_init;
    uint16_t direct_mapping_size_gb;
    uint16_t kernel_size_pages;
} vmm_globals;

/// VMM GLOBALS

pagetable_context _km_vmm_ctx;

vmm_globals _km_vmm_globals = {
    .is_post_init = FALSE,
    .direct_mapping_size_gb = 8,
    .kernel_size_pages = 512
};

// VMM Utilities

ptable_index_t offset_to_index(void* address, PTABLE_LVL lvl) {
    switch (lvl)
    {
    case PTABLE_LVL_PML4:
        return (((uint64_t)address >> (12 + 9 * 3)) & 0x1ff);
    case PTABLE_LVL_PDPT:
        return (((uint64_t)address >> (12 + 9 * 2)) & 0x1ff);
    case PTABLE_LVL_PDE:
        return (((uint64_t)address >> (12 + 9)) & 0x1ff);
    case PTABLE_LVL_PTE:
        return (((uint64_t)address >> 12) & 0x1ff);
    }

    return (ptable_index_t)-1;
}

void* vphys_address(void* phys) {
    return (void*)((uint64_t)phys | ((_km_vmm_globals.is_post_init) ? (RAM_DIRECT_MAPPING_OFFSET) : 0));
}

void* canonify(void* address) {
    return ((uint64_t) address & (1ull << 47)) ? 
        (void*)((0xffffull << 48) | (uint64_t)address) :
        (void*)((~(0xffffull << 48)) & (uint64_t)address);
}

// PTE

pte_t* pte_get_index(pte_t* table, ptable_index_t index) { 
    if (index >= ENTRIES_PER_TABLE) return NULL;

    return (pte_t*)vphys_address(table + index);
}

void pte_put(pte_t* table, ptable_index_t index, pte_t value) { 
    if (index >= ENTRIES_PER_TABLE) return;

    *((pte_t*)vphys_address(table  + index)) = value;
}

void pte_apply_options_to_entry(pte_t* entry, PAGE_MAPPING_OPTIONS options) {
    if (options & PAGE_MAPPING_USERSPACE) {
        mark_user_space(entry);
    }
    if (options & PAGE_MAPPING_HUGE_PAGE) {
        mark_huge_page(entry);
    }
    if (options & PAGE_MAPPING_NON_CACHEABLE) {
        mark_non_cacheable(entry);
    }
}

pte_t pte_create_entry(void* phys_address, PAGE_MAPPING_OPTIONS options) {
    pte_t entry = DEFAULT_PTE;
    pte_apply_options_to_entry(&entry, options);
    assign_address(&entry, phys_address);
    return entry;
}

// PT 

pte_t* pt_allocate_into(pte_t* table_index_p, PAGE_MAPPING_OPTIONS options) {
    void* phys = pmm_alloc();
    if (!phys) return NULL;

    pte_t* ptable = (pte_t*)((vphys_address(phys)));
    memset((void*)ptable, 0, PAGE_SIZE);

    (*table_index_p) = pte_create_entry(phys, options);

    return ptable;
}

pte_t* pt_get_or_allocate_into(pte_t* table_index_p, PAGE_MAPPING_OPTIONS options) {
    if (!(*table_index_p)) {
        return pt_allocate_into(table_index_p, options);
    }

    pte_t* pte = (pte_t*)vphys_address(pte_get_address(table_index_p));
    pte_apply_options_to_entry(pte, options);

    return pte;
}

// VMM

pte_t* vmm_init_page_entry(pagetable_context* ctx, void* vaddr, PTABLE_LVL level, PAGE_MAPPING_OPTIONS options) {    
    vaddr = ALIGN_4KB(vaddr);
    if (vaddr == NULL) return NULL;

    pte_t* pml4_entry = pte_get_index(ctx->root_table, offset_to_index(vaddr, PTABLE_LVL_PML4));
    pte_t* pdpt = pt_get_or_allocate_into(pml4_entry, options);
    if (!pdpt) return NULL;

    pte_t* pdpt_entry = pte_get_index(pdpt, offset_to_index(vaddr, PTABLE_LVL_PDPT));
    if (level == PTABLE_LVL_PDPT) return pdpt_entry;
    
    pte_t* pde = pt_get_or_allocate_into(pdpt_entry, options);
    if (!pde) return NULL;

    pte_t* pde_entry = pte_get_index(pde, offset_to_index(vaddr, PTABLE_LVL_PDE));
    if (level == PTABLE_LVL_PDE) return pde_entry;

    pte_t* pt = pt_get_or_allocate_into(pde_entry, options);
    if (!pt) return NULL;

    pte_t* pte = pte_get_index(pt, offset_to_index(vaddr, PTABLE_LVL_PTE));

    return pte;
}

void vmm_free_page_entry(void* vaddr) { 
    vaddr = ALIGN_4KB(vaddr);
    if (vaddr== NULL) return;

    
}

ERROR_CODE vmm_map(pagetable_context* ctx, void* vaddr, void* paddr, PAGE_MAPPING_OPTIONS options) {
    pte_t* entry = vmm_init_page_entry(ctx, vaddr, PTABLE_LVL_PTE, options);

    if (entry == NULL) return FAILED;

    (*entry) = pte_create_entry(paddr, options);

    flush_tlb((uint64_t)vaddr);
    
    return SUCCESS;
}

void* vmm_map_mmio_region(pagetable_context* ctx, void* phys_start, void* phys_end) {
    phys_start = ALIGN_4KB(phys_start);
    phys_end = ALIGN_4KB(phys_end);
    void* vaddr = (void*)(RAM_MMIO_MAPPING_OFFSET | (uint64_t)phys_start);
    void* current_vaddr = vaddr;

    while ((uint64_t)phys_start < (uint64_t)phys_end)
    {
        pte_t* entry = vmm_init_page_entry(ctx, current_vaddr, PTABLE_LVL_PTE, PAGE_MAPPING_DEFAULT);

        if (entry == NULL) return NULL;

        (*entry) = pte_create_entry(phys_start, PAGE_MAPPING_NON_CACHEABLE);
        flush_tlb((uint64_t)current_vaddr);
        
        phys_start = (void*)((uint64_t)phys_start + PAGE_SIZE);
        current_vaddr = (void*)((uint64_t)current_vaddr + PAGE_SIZE);
    }

    return vaddr;
}

pagetable_context* vmm_get_global_context() {
    return &_km_vmm_ctx;
}

/// VMM USERSPACE

pagetable_context* vmm_create_userspace_context() {
    pagetable_context* ctx = (pagetable_context*)kmalloc(sizeof(pagetable_context));
    if (!ctx) return NULL;

    ctx->root_table = (pte_t*)pmm_alloc(PAGE_SIZE);
    
    for (uint16_t i = ENTRIES_PER_TABLE / 2; i < ENTRIES_PER_TABLE; i++)
    {
        *pte_get_index(ctx->root_table, i) = *pte_get_index(_km_vmm_ctx.root_table, i);
    }

    return ctx;
}

size_t vmm_allocate_umm(pagetable_context* ctx, uint64_t vaddress, size_t len) {
    vaddress = (uint64_t)ALIGN_4KB(vaddress);
    len = (size_t)ALIGN_4KB(len);

    if (vaddress + len >= HIGHER_HALF_KERNEL_OFFSET) {
        return 0;
    }

    for (uint64_t i = vaddress; i < vaddress + len; i += PAGE_SIZE)
    {
        void* phys = pmm_alloc();
        if (!phys) return i - vaddress;

        vmm_map(ctx, (void*)i, phys, PAGE_MAPPING_USERSPACE);
    }

    return len;
}

void vmm_apply_pagetable(pagetable_context* ctx) {
    write_cr3((uint64_t)ctx->root_table);
}

// INITIALIZATION

void direct_map_gigabytes(pagetable_context* ctx, uint16_t count) {
    const uint64_t GIGABYTE = 1024 * 1024 * 1024;
    
    uint8_t* vstart_addr = (uint8_t*)RAM_DIRECT_MAPPING_OFFSET;

    for (uint16_t i = 0; i < count; i++, vstart_addr += GIGABYTE)
    {
        pte_t* pdpt_entry = vmm_init_page_entry(ctx, vstart_addr, PTABLE_LVL_PDPT, PAGE_MAPPING_DEFAULT);
        (*pdpt_entry) = pte_create_entry((void*)(GIGABYTE * i), PAGE_MAPPING_HUGE_PAGE);
    }
}

void direct_map_kernel(pagetable_context* ctx) {
    uint8_t* current_vaddr = (uint8_t*)HIGHER_HALF_KERNEL_OFFSET;
    uint8_t* current_paddr = (uint8_t*)0x0;

    for (uint16_t i = 0; i < _km_vmm_globals.kernel_size_pages; i++)
    {
        if (!vmm_map(ctx, (void*)current_vaddr, (void*)current_paddr, PAGE_MAPPING_DEFAULT)) {
            qemu_log("Failed to map kernel");
            return;
        }
        
        current_paddr += PAGE_SIZE;
        current_vaddr += PAGE_SIZE;
    }
} 

void setup_higher_half_pml4(pte_t* pml4) {
    for (uint16_t i = ENTRIES_PER_TABLE / 2; i < ENTRIES_PER_TABLE; i++)
    {        
        if(!pt_get_or_allocate_into(pte_get_index(pml4, i), PAGE_MAPPING_DEFAULT)) {
            qemu_logf("Failed to allocate higher half pml4 entry %d", i);
            return;
        }
    }
}


ERROR_CODE init_vmem() {
    _km_vmm_ctx.root_table = (pte_t*)pmm_alloc();
    if (!_km_vmm_ctx.root_table) return FAILED;

    memset((void*)vphys_address(_km_vmm_ctx.root_table), 0, PAGE_SIZE);
    
    direct_map_gigabytes(&_km_vmm_ctx, _km_vmm_globals.direct_mapping_size_gb);
    direct_map_kernel(&_km_vmm_ctx);
    
    pmm_load_root_ptable(_km_vmm_ctx.root_table);

    _km_vmm_globals.is_post_init = TRUE;

    setup_higher_half_pml4(_km_vmm_ctx.root_table);

    return SUCCESS;
}