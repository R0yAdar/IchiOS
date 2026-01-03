#include "vmm.h"
#include "pmm.h"
#include "pte.h"
#include "print.h"
#include "cstring.h"
#include "assembly.h"

#define ENTRIES_PER_TABLE 512
#define RAM_DIRECT_MAPPING_OFFSET 0xffff800000000000
#define RAM_MMIO_MAPPING_OFFSET 0xffff900000000000
#define ALIGN_4KB(ptr) ((void*)((uint64_t)(ptr) & (~0xFFF)))

typedef enum {
    PTABLE_LVL_PML4 = 4,
    PTABLE_LVL_PDPT = 3,
    PTABLE_LVL_PDE = 2,
    PTABLE_LVL_PTE = 1
} PTABLE_LVL;

pte_t* _root_table;
BOOL _is_post_init = FALSE;

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

    return (ptable_index_t)-1;
}

void* vphys_address(void* phys) {
    return (void*)((uint64_t)phys | ((_is_post_init) ? (RAM_DIRECT_MAPPING_OFFSET) : 0));
}

pte_t* pte_get_index(pte_t* table, ptable_index_t index) { 
    // ASSERT index <= 512

    return (pte_t*)vphys_address(table + index);
}

void pte_put(pte_t* table, ptable_index_t index, pte_t value) { 
    // ASSERT index <= 512

    *((pte_t*)vphys_address(table  + index)) = value;
}

void* canonify(void* address) {
    return ((uint64_t) address & (1ull << 47)) ? 
        (void*)((0xffffull << 48) | (uint64_t)address) :
        (void*)((~(0xffffull << 48)) & (uint64_t)address);
}

pte_t* pt_allocate_into(pte_t* table_index_p) {
    pte_t entry = DEFAULT_PTE;

    mark_user_space(&entry); // TODO: ...

    void* phys = pmm_alloc(); // TODO: check null
    pte_t* ptable = (pte_t*)((vphys_address(phys)));

    memset((void*)ptable, 0, PAGE_SIZE);
    
    assign_address(&entry, phys);

    (*table_index_p) = entry;

    return ptable;
}

pte_t* pt_get_or_allocate_into(pte_t* table_index_p) {
    pte_t* addr;

    if (!(*table_index_p)) {
        addr = pt_allocate_into(table_index_p);
    }
    else {
        addr = (pte_t*)vphys_address(pte_get_address(table_index_p));
    }

    return addr;
}

pte_t* init_mapping_entry(void* vaddr, void* paddr) {    
    vaddr = ALIGN_4KB(vaddr);
    paddr = ALIGN_4KB(paddr);

    pte_t* pml4_entry = pte_get_index(_root_table, offset_to_index(vaddr, PTABLE_LVL_PML4));
    pte_t* pdpt = pt_get_or_allocate_into(pml4_entry);

    pte_t* pdpt_entry = pte_get_index(pdpt, offset_to_index(vaddr, PTABLE_LVL_PDPT));
    pte_t* pde = pt_get_or_allocate_into(pdpt_entry);
    
    pte_t* pde_entry = pte_get_index(pde, offset_to_index(vaddr, PTABLE_LVL_PDE));
    pte_t* pt = pt_get_or_allocate_into(pde_entry);

    pte_t* pte = pte_get_index(pt, offset_to_index(vaddr, PTABLE_LVL_PTE));
    
    return pte;
}

ERROR_CODE simple_map(void* vaddr, void* paddr) {
    pte_t* entry = init_mapping_entry(vaddr, paddr);

    if (entry == NULL) return FAILED;

    (*entry) = DEFAULT_PTE;
    assign_address(entry, paddr);

    flush_tlb((uint64_t)vaddr);
    
    return SUCCESS;
}

ERROR_CODE um_map(void* vaddr, void* paddr) {
    pte_t* entry = init_mapping_entry(vaddr, paddr);

    if (entry == NULL) return FAILED;

    (*entry) = DEFAULT_PTE;

    mark_user_space(entry);

    assign_address(entry, paddr);
    
    flush_tlb((uint64_t)vaddr);
    pmm_load_root_ptable(_root_table);

    return SUCCESS;
}


void direct_map_gigabytes(uint16_t count) {
    const uint64_t GIGABYTE = 1024 * 1024 * 1024;
    
    uint8_t* vstart_addr = (uint8_t*)RAM_DIRECT_MAPPING_OFFSET;

    for (uint16_t i = 0; i < count; i++, vstart_addr += GIGABYTE)
    {
        pte_t* pml4_entry = pte_get_index(_root_table, offset_to_index(vstart_addr, PTABLE_LVL_PML4));
        pte_t* pdpt = pt_get_or_allocate_into(pml4_entry);
        pte_t* pdpt_entry = pte_get_index(pdpt, offset_to_index(vstart_addr, PTABLE_LVL_PDPT));
        (*pdpt_entry) = DEFAULT_HUGE_PTE;
        assign_address(pdpt_entry, (void*)(GIGABYTE * i));
    }
}

void direct_map_kernel() {
    uint8_t* current_vaddr = (uint8_t*)HIGHER_HALF_KERNEL_OFFSET;
    uint8_t* current_paddr = (uint8_t*)0x0;

    for (uint16_t i = 0; i < 512; i++)
    {
        simple_map((void*)current_vaddr, (void*)current_paddr);
        
        current_paddr += PAGE_SIZE;
        current_vaddr += PAGE_SIZE;
    }
} 

void* map_mmio_region(void* phys_start, void* phys_end) {
    phys_start = ALIGN_4KB(phys_start);
    phys_end = ALIGN_4KB(phys_end);
    void* vaddr = (void*)(RAM_MMIO_MAPPING_OFFSET | (uint64_t)phys_start);
    void* current_vaddr = vaddr;

    while ((uint64_t)phys_start < (uint64_t)phys_end)
    {
        pte_t* entry = init_mapping_entry(current_vaddr, phys_start);

        if (entry == NULL) return NULL;

        (*entry) = DEFAULT_PTE;
        
        mark_non_cacheable(entry);
        assign_address(entry, phys_start);
        flush_tlb((uint64_t)current_vaddr);
        
        phys_start = (void*)((uint64_t)phys_start + PAGE_SIZE);
        current_vaddr = (void*)((uint64_t)current_vaddr + PAGE_SIZE);
    }

    return vaddr;
}

ERROR_CODE init_vmem() {
    _root_table = (pte_t*)pmm_alloc();
    memset((void*)_root_table, 0, PAGE_SIZE);

    direct_map_gigabytes(8);
    direct_map_kernel();
    
    pmm_load_root_ptable(_root_table);

    _is_post_init = TRUE;

    return SUCCESS;
}

/*
void* vmem_lookup_paddress(void* virt) {
    // physical addresses are limited to 52
    //uint64_t non_canonical_address = ((uint64_t)virt) << 12;
    //pte_t* next_table = (pte_t*)vphys_address(pte_get_address(_root_table + (non_canonical_address & 0x1FF)));
}
*/

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
    void* vaddr = map_mmio_region(paddr, (void*)((uint64_t)paddr + PAGE_SIZE * page_count));
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

/*
    KMALLOC
*/

/* Structures */

#pragma pack (push, 1)

typedef struct
{
    uint8_t free_count;
    uint8_t total_count;
    uint16_t compressed_length;
} kmalloc_entry_metadata_t;

#define KMALLOC_FREELIST_PAGE_MAX_ADDR 500

typedef struct kmalloc_freelist_page
{
    void* addresses[KMALLOC_FREELIST_PAGE_MAX_ADDR];
    uint16_t count;
    struct kmalloc_freelist_page* next;
} kmalloc_freelist_page_t;


#pragma pack (pop)

/* Utility functions */

// local addresses are from 0xffff800000000000 onwards
uint32_t _km_compress_4kb_aligned_local_address(void* ptr) {
    return (uint32_t)((uint64_t)ptr >> 12);
}

void* _km_decompress_4kb_aligned_local_address(uint32_t caddr) {
    return (void*)((((uint64_t)caddr) << 12) | RAM_DIRECT_MAPPING_OFFSET);
}

uint16_t _kmalloc_fl_roundup_2power(uint16_t length) {
    // assert length <= 0x800
    uint16_t mask = 0x800;

    while (!(length & mask)) mask = mask >> 1;

    if (length & (~mask)) return mask << 1;
    else return mask;
}

uint8_t _kmalloc_log2(uint16_t size) {
    // optimize with tzbcnt
    uint8_t zero_bits_count = 0;
    
    while(!(size & 0x1)) {
        size = size >> 1;
        ++zero_bits_count;
    }

    return zero_bits_count;
}

uint8_t _kmalloc_fl_size_to_index(uint16_t size) {
    // we start from 8 --> 1000, so index 3 should be zero
    return _kmalloc_log2(size) - 3;
}

void* get_4kb_aligned_address(void* addr) { 
    return (void*)((uint64_t)addr & (~(0xFFF)));
}

/* Internal Definitions */

#define KMALLOC_SMALL_BLOCK_MAX_SIZE 2048
#define MINIMUM_BLOCK_SIZE 8

/* Internal Functions */

uint16_t _kmalloc_split_page_block_length[] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048};

uint64_t ke_get_block_length(kmalloc_entry_metadata_t* metadata) {
    if (metadata->compressed_length < sizeof(_kmalloc_split_page_block_length)) {
        return _kmalloc_split_page_block_length[metadata->compressed_length];
    }
    else {
        return (metadata->compressed_length - (sizeof(_kmalloc_split_page_block_length) - 1)) * PAGE_SIZE;
    }
}

// length is a power of 2
void ke_set_block_length(kmalloc_entry_metadata_t* metadata, uint64_t length) {
    if (length <= (PAGE_SIZE / 2)) {
        metadata->compressed_length = _kmalloc_log2((uint16_t)length) - 3; // we start from 2^3 = 8
    } else {
        metadata->compressed_length = (length / PAGE_SIZE) + (sizeof(_kmalloc_split_page_block_length) - 1);
    }
}

kmalloc_entry_metadata_t* _ke_get_metadata(void* addr) {
    return (kmalloc_entry_metadata_t*)get_4kb_aligned_address(addr);
}

// Allocation Sizes (Total 9)
// 8
// 16
// 32
// 64
// 128
// 256
// 512
// 1024
// 2048
kmalloc_freelist_page_t* _kmalloc_freelist[9] = {0};

kmalloc_freelist_page_t* kmalloc_fl_page_create() {
    kmalloc_freelist_page_t* p = (kmalloc_freelist_page_t*)kpage_alloc(1);
    p->count = 0;
    p->next = NULL;
    for (size_t i = 0; i < KMALLOC_FREELIST_PAGE_MAX_ADDR; i++) {
        p->addresses[i] = NULL;
    }

    return p;
}

void _kmalloc_fl_add(uint16_t size, void* vaddr) {
    uint8_t index = _kmalloc_fl_size_to_index(size);
    kmalloc_freelist_page_t* current = _kmalloc_freelist[index];

    if (current == NULL){
        current = kmalloc_fl_page_create();
    }
    else if (current->count == KMALLOC_FREELIST_PAGE_MAX_ADDR) {
        kmalloc_freelist_page_t* new_page = kmalloc_fl_page_create();
        new_page->next = current;
        current = new_page;
    }

    current->addresses[current->count++] = vaddr;
    _kmalloc_freelist[index] = current;
}

void* _kmalloc_fl_get(uint16_t size) {
    uint8_t index = _kmalloc_fl_size_to_index(size);
    kmalloc_freelist_page_t* current = _kmalloc_freelist[index];

    if (current == NULL || current->count == 0) return NULL;

    void* addr = current->addresses[--current->count];

    if (current->count == 0) {
        kpage_free((void*)current, 1);
        _kmalloc_freelist[index] = NULL;
    }

    return addr;
}

// metadata block length is the base block length for the block, pretty useless. but not,
void _kmalloc_setup_page_memory(void* page, kmalloc_entry_metadata_t* metadata) {
    uint16_t available_memory = PAGE_SIZE - sizeof(kmalloc_entry_metadata_t);
    uint16_t next_block_size = ke_get_block_length(metadata);
    void* page_ptr = (void*)((uint8_t*)page + sizeof(kmalloc_entry_metadata_t));
    metadata->free_count = 0;
    metadata->total_count = 0;

    while (available_memory - next_block_size > 0)
    {
        ++metadata->total_count;
        _kmalloc_fl_add(next_block_size, page_ptr);
        
        page_ptr = (void*)((uint8_t*)page_ptr + next_block_size);
        available_memory -= next_block_size;

        if (available_memory - next_block_size < 0) {
            next_block_size /= 2;
        }

        if (next_block_size < 8) {
            break;
        }
    }

    *(kmalloc_entry_metadata_t*)page = *metadata;
}

uint16_t _kmalloc_figure_blocksize(void* ptr, kmalloc_entry_metadata_t* metadata) {
    void* page = get_4kb_aligned_address(ptr);
    uint16_t available_memory = PAGE_SIZE - sizeof(kmalloc_entry_metadata_t);
    uint16_t next_block_size = ke_get_block_length(metadata);
    void* page_ptr = (void*)((uint8_t*)page + sizeof(kmalloc_entry_metadata_t));

    while (available_memory - next_block_size > 0)
    {
        if (ptr == page_ptr) {
            return next_block_size;
        }

        ++metadata->total_count;
        
        page_ptr = (void*)((uint8_t*)page_ptr + next_block_size);
        available_memory -= next_block_size;

        if (available_memory - next_block_size < 0) {
            next_block_size /= 2;
        }

        if (next_block_size < 8) {
            break;
        }
    }

    return 0;
}

/* API */

void* kmalloc(size_t len) {
    size_t mem_length = sizeof(kmalloc_entry_metadata_t) + len;
    kmalloc_entry_metadata_t metadata = {0};

    if (mem_length > PAGE_SIZE / 2) {
        uint32_t page_count = (mem_length + PAGE_SIZE - 1) / PAGE_SIZE;

        void* vaddr = kpage_alloc(page_count);
        if (!vaddr) {
            return vaddr;
        }

        // special case -> we don't need to store metadata!!!
        if (page_count == 1) { 
            return vaddr;
        }

        ke_set_block_length(&metadata, page_count * PAGE_SIZE);

        *(kmalloc_entry_metadata_t*)(vaddr) = metadata;
        return (void*)((kmalloc_entry_metadata_t*)(vaddr) + 1);
    }

    if (len < MINIMUM_BLOCK_SIZE) len = MINIMUM_BLOCK_SIZE;
    len = _kmalloc_fl_roundup_2power(len);

    ke_set_block_length(&metadata, len);
    //uint8_t index = _kmalloc_fl_size_to_index(len);

    void* addr = _kmalloc_fl_get(len);

    if (addr != NULL) {
        --_ke_get_metadata(addr)->free_count;
        return addr;
    }

    void* page = kpage_alloc(1);

    if(!page) return NULL;

    _kmalloc_setup_page_memory(page, &metadata);  
    
    return kmalloc(len);
}

void kfree(void* vaddr) {
    if (!vaddr) return;

    if (get_4kb_aligned_address(vaddr) == vaddr) {
        kpage_free((void*)vaddr, 1);
    }

    kmalloc_entry_metadata_t* metadata_ptr = _ke_get_metadata(vaddr);

    uint64_t block_length = ke_get_block_length(metadata_ptr);

    if (block_length >= PAGE_SIZE) {
        kpage_free((void*)vaddr, block_length / PAGE_SIZE);
        return;
    }

    // TODO if (metadata_ptr->free_count + 1) == metadata_ptr->total_count ---> free whole page
    
    _kmalloc_fl_add(_kmalloc_figure_blocksize(vaddr, metadata_ptr), vaddr);
    ++metadata_ptr->free_count;
}

/*
    USER SPACE
*/

typedef struct
{
    void* vaddr;
    void* paddr;
    size_t size;
} memory_chunk;


struct userspace_context
{
    memory_chunk* mem_chunks;
};


// returns the length of allocated memory, expects 4kb aligned numbers
size_t allocate_umm(uint64_t vaddr_start, size_t len) {
    vaddr_start = (uint64_t)ALIGN_4KB(vaddr_start);
    len = (size_t)ALIGN_4KB(len);

    if (vaddr_start + len >= HIGHER_HALF_KERNEL_OFFSET) {
        return 0;
    }

    for (uint64_t i = vaddr_start; i < vaddr_start + len; i += PAGE_SIZE)
    {
        void* phys = pmm_alloc();
        if (!phys) return i - vaddr_start;
        um_map((void*)i, phys);
    }

    return len;
}