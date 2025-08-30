#include "vmm.h"
#include "pmm.h"
#include "pte.h"
#include "print.h"
#include "cstring.h"

#define ENTRIES_PER_TABLE 512
#define RAM_DIRECT_MAPPING_OFFSET 0xffff800000000000

typedef enum {
    PTABLE_LVL_PML4 = 4,
    PTABLE_LVL_PDPT = 3,
    PTABLE_LVL_PDE = 2,
    PTABLE_LVL_PTE = 1
} PTABLE_LVL;

pte_t* _root_table;

typedef uint16_t ptable_index_t;

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
}

pte_t* pte_get_index(pte_t* table, ptable_index_t index) { 
    // ASSERT index <= 512

    return table  + index;
}

void pte_put(pte_t* table, ptable_index_t index, pte_t value) { 
    // ASSERT index <= 512

    *(table  + index) = value;
}

void print_table(pte_t* t, int level) {
    if (level == 0) {
        return;
    }

    int count = 0;

    print("Table starting at: "); printxln((void*)t);

    for (int i = 0; i < ENTRIES_PER_TABLE; i++)
    {
        if (*(t + i) != 0) {
            print("Entry at: "); printx(i);
            print(" pointing to -> "); printxln((uint64_t)get_address((t + i)));
            ++count;
            
            print_table((pte_t*)get_address((t + i)), level - 1);
        }

        if (count > 3) {
            return;
        }
    }
}

void* canonify(void* address) {
    return ((uint64_t) address & (1ull << 47)) ? 
        (void*)((0xffffull << 48) | (uint64_t)address) :
        (void*)((~(0xffffull << 48)) & (uint64_t)address);
}

typedef enum  {
    PTABLE_MAP_LEN_4KB
} PTABLE_MAP_LEN;

pte_t* __early_pt_allocate_into(pte_t* table_index_p) {
    pte_t entry = DEFAULT_PTE;

    pte_t* ptable = (pte_t*)pmm_alloc();
    memset((void*)ptable, 0, PAGE_SIZE);
    
    assign_address(&entry, ptable);

    (*table_index_p) = entry;

    return ptable;
}

pte_t* pt_get_or_allocate_into(pte_t* table_index_p) {
    pte_t* addr;

    if (!(*table_index_p)) {
        addr = __early_pt_allocate_into(table_index_p);
    }
    else {
        addr = (pte_t*)get_address(table_index_p);
    }

    return addr;
}

#define ALIGN_4KB(ptr) ((void*)((uint64_t)(ptr) & (~0xFFF)))

ERROR_CODE simple_map(void* vaddr, void* paddr) {
    // can also assert
    vaddr = ALIGN_4KB(vaddr);
    paddr = ALIGN_4KB(paddr);

    pte_t* pml4_entry = pte_get_index(_root_table, offset_to_index(vaddr, PTABLE_LVL_PML4));
    pte_t* pdpt = pt_get_or_allocate_into(pml4_entry);

    pte_t* pdpt_entry = pte_get_index(pdpt, offset_to_index(vaddr, PTABLE_LVL_PDPT));
    pte_t* pde = pt_get_or_allocate_into(pdpt_entry);
    
    pte_t* pde_entry = pte_get_index(pde, offset_to_index(vaddr, PTABLE_LVL_PDE));
    pte_t* pt = pt_get_or_allocate_into(pde_entry);

    pte_t* pte = pte_get_index(pt, offset_to_index(vaddr, PTABLE_LVL_PTE));
    
    (*pte) = DEFAULT_PTE;
    assign_address(pte, paddr);
}

void direct_map_1gb() {
    void* vstart_addr = (uint8_t*)RAM_DIRECT_MAPPING_OFFSET;

    pte_t* pml4_entry = pte_get_index(_root_table, offset_to_index(vstart_addr, PTABLE_LVL_PML4));
    pte_t* pdpt = pt_get_or_allocate_into(pml4_entry);
    pte_t* pdpt_entry = pte_get_index(pdpt, offset_to_index(vstart_addr, PTABLE_LVL_PDPT));
    (*pdpt_entry) = DEFAULT_HUGE_PTE;
    assign_address(pdpt_entry, 0x0);
}

void direct_map_kernel() {
    char* current_vaddr = (char*)0xffffffff80000000;
    char* current_paddr = (char*)0x0;

    for (uint16_t i = 0; i < 512; i++)
    {
        simple_map((void*)current_vaddr, (void*)current_paddr);
        
        current_paddr += PAGE_SIZE;
        current_vaddr += PAGE_SIZE;
    }
} 

// creates new tables and maps first 2MB of memory to higher half kernel space
ERROR_CODE init_vmem() {
    _root_table = (pte_t*)pmm_alloc();
    memset((void*)_root_table, 0, PAGE_SIZE);

    direct_map_1gb();
    direct_map_kernel();
    
    pmm_load_root_ptable(_root_table);

    return SUCCESS;
}

void* vmem_allocate_page() {

}

void vmem_free_page() {

}

void* vmem_lookup_paddress(void* virt) {
    // physical addresses are limited to 52
    uint64_t non_canonical_address = ((uint64_t)virt) << 12;    
    pte_t* next_table = (pte_t*)get_address(_root_table + (non_canonical_address & 0x1FF));
}

void vmem_map_page(void* phys, void* virt) {

}

void* kmalloc(size_t len) {

}
