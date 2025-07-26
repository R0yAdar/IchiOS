#include "pmm.h"
#include "cstring.h"

typedef enum block_status_t {
    BLOCK_STATUS_FREE = 0,
    BLOCK_STATUS_USED = 1,
    BLOCK_STATUS_8CHUNK_USED = 0xff
} block_status;

#define BITMAP_CELL_BIT_SIZE 32

// global static variables
// Maximum of 512MB
uint32_t bitmap[4096];
pmm_context _context;

uint32_t set_bit(uint32_t value, uint32_t index) {
    return value | (1 << index);
}

uint32_t free_bit(uint32_t value, uint32_t index) {
    return value | (~(1 << index));
}

uint32_t find_first_free_bit(uint32_t value) {
     for (int i = 0; i < sizeof(value) * 8; ++i) {
        if (!((value >> i) & 1)) { 
            return i;
        }
    }

    return -1;
}

int pmm_init(pmm_context* context){
    _context = *context;

    // whole map starts as used
    memset(&bitmap, BLOCK_STATUS_8CHUNK_USED, sizeof(bitmap));

    for (uint32_t i = 0; i < context->regions_count; i++)
    {
        memory_region* current = context->regions + i;

        if (current->type == MEMORY_REGION_AVAILABLE) {
            uint32_t index_in_bitmap = (current->address / 8) / 4096;
            uint32_t count_in_bitmap = current->size / 4096 / 8;

            memset(&bitmap + index_in_bitmap, BLOCK_STATUS_FREE, count_in_bitmap);
        }
    }
    
    // reserve start of memory for kernel...
    memset(&bitmap, BLOCK_STATUS_8CHUNK_USED, RESERVED_KERNEL_MEMORY_BYTES / 8 / 4096);

    return 0;
}

void* pmm_alloc() {
    for(uint32_t i = 0; i < LENGTH(bitmap); ++i) {
        if (bitmap[i] != BLOCK_STATUS_8CHUNK_USED) {
            uint32_t free_bit_index = find_first_free_bit(bitmap[i]);
            bitmap[i] = set_bit(bitmap[i], free_bit_index);
            return i * 8 * 4096 + free_bit_index * 4096;
        }
    }
    
    return NULL;
}

void pmm_free(void* page) {
    uint32_t index_in_bitmap = (uint64_t)page / PMM_BLOCK_SIZE / BITMAP_CELL_BIT_SIZE;
    uint32_t bit_index = ((uint64_t)page / PMM_BLOCK_SIZE) % BITMAP_CELL_BIT_SIZE;
    bitmap[index_in_bitmap] = free_bit(bitmap[index_in_bitmap], bit_index);
}

// void*   pmm_alloc_blocks(uint32_t count);

// void    pmm_free_blocks(void* start, uint32_t count);
