#include "vmm.h"
#include "pmm.h"
#include "serial.h"

/// KMALLOC

/// Definitions

#define KMALLOC_SMALL_BLOCK_MAX_SIZE 2048
#define KMALLOC_MINIMUM_BLOCK_SIZE 8
#define KMALLOC_FIXED_BLOCK_BRACKETS_COUNT 9

/// Structures

#pragma pack(push, 1)

typedef struct
{
    uint8_t free_count;
    uint8_t total_count;
    uint16_t compressed_length;
} kmalloc_entry_metadata_t;

#define KMALLOC_FREELIST_PAGE_MAX_ADDR 500

typedef struct kmalloc_freelist_page
{
    void *addresses[KMALLOC_FREELIST_PAGE_MAX_ADDR];
    uint16_t count;
    struct kmalloc_freelist_page *next;
} kmalloc_freelist_page_t;

#pragma pack(pop)

/// Utility functions

// Local addresses are from 0xffff800000000000 onwards
uint32_t km_compress_4kb_aligned_local_address(void *ptr)
{
    return (uint32_t)((uint64_t)ptr >> 12);
}

void *km_decompress_4kb_aligned_local_address(uint32_t caddr)
{
    return (void *)((((uint64_t)caddr) << 12) | VMM_RAM_DIRECT_MAPPING_OFFSET);
}

uint16_t km_fl_roundup_2power(uint16_t length)
{
    // Assert length <= 0x800
    uint16_t mask = 0x800;

    while (!(length & mask))
        mask = mask >> 1;

    if (length & (~mask))
        return mask << 1;
    else
        return mask;
}

// Can optimize with tzbcnt on x86
uint8_t km_log2(uint16_t size)
{
    uint8_t zero_bits_count = 0;

    while (!(size & 0x1))
    {
        size = size >> 1;
        ++zero_bits_count;
    }

    return zero_bits_count;
}

// The minimum size is 8, 0b1000 so index 3 should be zero
uint8_t km_fl_size_to_index(uint16_t size)
{
    return km_log2(size) - 3;
}

void *km_4kb_aligned_address(void *addr)
{
    return (void *)((uint64_t)addr & (~(0xFFF)));
}

/// Implementation

uint16_t _kmalloc_split_page_block_length[] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048};

uint64_t ke_get_block_length(kmalloc_entry_metadata_t *metadata)
{
    if (metadata->compressed_length < sizeof(_kmalloc_split_page_block_length))
    {
        return _kmalloc_split_page_block_length[metadata->compressed_length];
    }
    else
    {
        return (metadata->compressed_length - (sizeof(_kmalloc_split_page_block_length) - 1)) * PAGE_SIZE;
    }
}

// Length is a power of 2
void ke_set_block_length(kmalloc_entry_metadata_t *metadata, uint64_t length)
{
    if (length <= (PAGE_SIZE / 2))
    {
        metadata->compressed_length = km_fl_size_to_index((uint16_t)length);
    }
    else
    {
        metadata->compressed_length = (length / PAGE_SIZE) + (sizeof(_kmalloc_split_page_block_length) - 1);
    }
}

kmalloc_entry_metadata_t *ke_get_metadata(void *addr)
{
    return (kmalloc_entry_metadata_t *)km_4kb_aligned_address(addr);
}

/// Free-list (LEN=9, for each block-size)

kmalloc_freelist_page_t *_kmalloc_freelist[KMALLOC_FIXED_BLOCK_BRACKETS_COUNT] = {0};

kmalloc_freelist_page_t *km_fl_page_create()
{
    kmalloc_freelist_page_t *p = (kmalloc_freelist_page_t *)kpage_alloc(1);
    if (!p)
        return NULL;

    p->count = 0;
    p->next = NULL;

    for (size_t i = 0; i < KMALLOC_FREELIST_PAGE_MAX_ADDR; i++)
    {
        p->addresses[i] = NULL;
    }

    return p;
}

void km_fl_add(uint16_t size, void *vaddr)
{
    uint8_t index = km_fl_size_to_index(size);
    kmalloc_freelist_page_t *current = _kmalloc_freelist[index];

    if (current == NULL)
    {
        current = km_fl_page_create();
    }
    else if (current->count == KMALLOC_FREELIST_PAGE_MAX_ADDR)
    {
        kmalloc_freelist_page_t *new_page = km_fl_page_create();
        new_page->next = current;
        current = new_page;
    }

    current->addresses[current->count++] = vaddr;
    _kmalloc_freelist[index] = current;
}

void *km_fl_get(uint16_t size)
{
    uint8_t index = km_fl_size_to_index(size);
    kmalloc_freelist_page_t *current = _kmalloc_freelist[index];

    if (current == NULL || current->count == 0)
        return NULL;

    void *addr = current->addresses[--current->count];

    if (current->count == 0 && current->next != NULL)
    {
        _kmalloc_freelist[index] = current->next;
        kpage_free((void *)current, 1);
    }

    return addr;
}

void km_setup_page_memory(void *page, kmalloc_entry_metadata_t *metadata)
{
    uint16_t available_memory = PAGE_SIZE - sizeof(kmalloc_entry_metadata_t);
    uint16_t next_block_size = ke_get_block_length(metadata);
    void *page_ptr = (void *)((uint8_t *)page + sizeof(kmalloc_entry_metadata_t));
    metadata->free_count = 0;
    metadata->total_count = 0;

    while (available_memory - next_block_size > 0)
    {
        ++metadata->total_count;
        ++metadata->free_count;
        km_fl_add(next_block_size, page_ptr);

        page_ptr = (void *)((uint8_t *)page_ptr + next_block_size);
        available_memory -= next_block_size;

        while (available_memory < next_block_size && next_block_size > 8)
        {
            next_block_size /= 2;
        }

        if (next_block_size < 8)
        {
            break;
        }
    }

    *(kmalloc_entry_metadata_t *)page = *metadata;
}

uint16_t km_figure_blocksize(void *ptr, kmalloc_entry_metadata_t *metadata)
{
    void *page = km_4kb_aligned_address(ptr);
    uint16_t available_memory = PAGE_SIZE - sizeof(kmalloc_entry_metadata_t);
    uint16_t next_block_size = ke_get_block_length(metadata);
    void *page_ptr = (void *)((uint8_t *)page + sizeof(kmalloc_entry_metadata_t));

    while (available_memory - next_block_size > 0)
    {
        if (ptr == page_ptr)
        {
            return next_block_size;
        }

        page_ptr = (void *)((uint8_t *)page_ptr + next_block_size);
        available_memory -= next_block_size;

        while (available_memory < next_block_size && next_block_size > 8)
        {
            next_block_size /= 2;
        }

        if (next_block_size < 8)
        {
            break;
        }
    }

    return 0;
}

/// API

void *kmalloc(size_t len)
{
    if (len == 0)
        return NULL;

    size_t mem_length = sizeof(kmalloc_entry_metadata_t) + len;
    kmalloc_entry_metadata_t metadata = {0};

    if (mem_length > PAGE_SIZE / 2)
    {
        uint32_t page_count = (mem_length + PAGE_SIZE - 1) / PAGE_SIZE;

        if (page_count == 1 || len <= PAGE_SIZE)
        {
            return kpage_alloc(1);
        }

        void *vaddr = kpage_alloc(page_count);

        if (!vaddr)
        {
            return NULL;
        }

        ke_set_block_length(&metadata, page_count * PAGE_SIZE);

        *(kmalloc_entry_metadata_t *)(vaddr) = metadata;

        return (void *)((kmalloc_entry_metadata_t *)(vaddr) + 1);
    }

    mem_length -= sizeof(kmalloc_entry_metadata_t);

    if (mem_length < KMALLOC_MINIMUM_BLOCK_SIZE)
    {
        mem_length = KMALLOC_MINIMUM_BLOCK_SIZE;
    }

    mem_length = km_fl_roundup_2power(mem_length);

    ke_set_block_length(&metadata, mem_length);

    void *addr = km_fl_get(mem_length);

    if (addr != NULL)
    {
        --ke_get_metadata(addr)->free_count;
        return addr;
    }

    void *page = kpage_alloc(1);

    if (!page)
    {
        return NULL;
    }

    km_setup_page_memory(page, &metadata);

    return kmalloc(len);
}

void kfree(void *vaddr)
{
    if (!vaddr)
    {
        return;
    }

    if (km_4kb_aligned_address(vaddr) == vaddr)
    {
        kpage_free((void *)vaddr, 1);
        return;
    }

    kmalloc_entry_metadata_t *metadata_ptr = ke_get_metadata(vaddr);

    uint64_t block_length = ke_get_block_length(metadata_ptr);

    if (block_length >= PAGE_SIZE)
    {
        kpage_free((void *)metadata_ptr, block_length / PAGE_SIZE);
        return;
    }

    km_fl_add(km_figure_blocksize(vaddr, metadata_ptr), vaddr);
    ++metadata_ptr->free_count;
}
