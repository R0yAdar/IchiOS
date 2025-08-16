#include "vmem.h"
#include "pmm.h"
#include "pte.h"
#include "print.h"
#include "cstring.h"

#define ENTRIES_PER_TABLE 512

pte_t* allocate_from_pmm() {
    pte_t* table = (pte_t*)pmm_alloc();
    memset((void*)table, 0, PAGE_SIZE);
    return table;
}

void print_table(pte_t* t) {
    int count = 0;

    for (int i = 0; i < ENTRIES_PER_TABLE; i++)
    {
        if (*(t + i) != 0) {
            ++count;
        }
    }
    
    print("Table starting at: "); printx((void*)t); print(" Count is: "); printiln(count);
}

// creates new tables and maps first 2MB of memory to higher half kernel space
ERROR_CODE init_vmem() {
    pte_t pte = DEFAULT_PTE;

    pte_t* pml4 = allocate_from_pmm();
    pte_t* level3 = allocate_from_pmm();
    pte_t* level2 = allocate_from_pmm();
    pte_t* level1 = allocate_from_pmm();

    assign_address(&pte, level3);

    *(pml4 + 0x1FF) = pte;
    
    assign_address(&pte, level2);

    *(level3 + 0x1FE) = pte;

    assign_address(&pte, level1);
    *level2 = pte;
    
    char* current_phys_address = (char*)0x0;

    for (uint16_t i = 0; i < PAGE_SIZE / sizeof(pte_t); i++)
    {
        assign_address(&pte, (void*)current_phys_address);

        *(level1++) = pte;
        
        current_phys_address += PAGE_SIZE;
    }
    
    pmm_load_root_ptable(pml4);
    
    return SUCCESS;
}

void* vmem_allocate_page() {

}

void vmem_free_page() {

}

void* vmem_lookup_vaddress(void* phys) {

}

void vmem_map_page(void* phys, void* virt) {

}

void vmem_flush_tlb_entry() {

}
