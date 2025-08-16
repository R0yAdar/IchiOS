#include "vmem.h"
#include "pmm.h"
#include "pte.h"
#include "print.h"
#include "cstring.h"

// creates new tables and maps first 2MB of memory to higher half kernel space
ERROR_CODE init_vmem() {
    pte_t* pml4 = (pte_t*)pmm_alloc();
    memset((void*)pml4, 0, PAGE_SIZE);
    
    pte_t pte = DEFAULT_PTE;

    pte_t* level3 = (pte_t*)pmm_alloc();
    memset((void*)level3, 0, PAGE_SIZE);
    assign_address(&pte, level3);
    

    *(pml4 + 0x1FF) = pte;
    
    pte_t* level2 = (pte_t*)pmm_alloc();
    memset((void*)level2, 0, PAGE_SIZE);
    assign_address(&pte, level2);

    *(level3 + 0x1FE) = pte;

    pte_t* level1 = (pte_t*)pmm_alloc();
    memset((void*)level1, 0, PAGE_SIZE);
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
