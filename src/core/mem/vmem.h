#include "err.h"
#include "multiboot.h"

ERROR_CODE init_vmem(multiboot_info* boot_info);

void*   vmem_allocate_page();

void    vmem_free_page();

void*   vmem_lookup_vaddress(void* phys);

void    vmem_map_page(void* phys, void* virt);

void    vmem_flush_tlb_entry();
