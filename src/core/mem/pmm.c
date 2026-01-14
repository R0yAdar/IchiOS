#include "pmm.h"
#include "cstring.h"
#include "assembly.h"
#include "print.h"
#include "serial.h"

#define BLOCK_STATUS_FREE 0
#define BLOCK_STATUS_USED 1
#define BLOCK_STATUS_8CHUNK_USED 0xff
#define BLOCK_STATUS_CHUNK_USED 0xffffffff

#define BITMAP_CELL_BIT_SIZE 32
#define PAGE_SIZE 4096

uint32_t bitmap[4096];
pmm_context _context;

uint32_t set_bit(uint32_t value, uint32_t index) {
    return value | (1 << index);
}

uint32_t free_bit(uint32_t value, uint32_t index) {
    return value & (~(1 << index));
}

int32_t find_first_free_bit(uint32_t value) {
     for (int i = 0; i < (int)sizeof(value) * 8; ++i) {
        if (!((value >> i) & 1)) { 
            return i;
        }
    }

    return -1;
}

uint64_t find_first_free_bits(uint32_t* values, uint64_t maxlen, uint32_t count) {
    uint32_t free_count = 0;

     for (uint64_t i = 0; i < maxlen * BITMAP_CELL_BIT_SIZE; ++i) {
        if (!((values[i / BITMAP_CELL_BIT_SIZE] >> (i % BITMAP_CELL_BIT_SIZE)) & 1)) {
            ++free_count;

            if (free_count == count) {
                return i - count + 1;
            }
        }
        else {
            free_count = 0;
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
    
    memset(&bitmap, BLOCK_STATUS_8CHUNK_USED, ((context->kernel_ram_size + 4095) / 4096 + 7) / 8);

    return 0;
}

void* pmm_alloc() {
    for(uint64_t i = 0; i < LENGTH(bitmap); ++i) {
        if (bitmap[i] != BLOCK_STATUS_CHUNK_USED) {
            int32_t free_bit_index = find_first_free_bit(bitmap[i]);

            if (free_bit_index == -1) { return NULL; }

            bitmap[i] = set_bit(bitmap[i], free_bit_index);
            return (void*)((i * BITMAP_CELL_BIT_SIZE + free_bit_index) * PAGE_SIZE);
        }
    }
    
    return NULL;
}

void pmm_free(void* block) {
    uint32_t index_in_bitmap = (uint64_t)block / PMM_BLOCK_SIZE / BITMAP_CELL_BIT_SIZE;
    uint32_t bit_index = ((uint64_t)block / PMM_BLOCK_SIZE) % BITMAP_CELL_BIT_SIZE;
    bitmap[index_in_bitmap] = free_bit(bitmap[index_in_bitmap], bit_index);
}

void* pmm_alloc_blocks(uint32_t count) {
    uint64_t index = find_first_free_bits(bitmap, LENGTH(bitmap), count);

    if (index == (uint64_t)-1) {
        qemu_log("No free memory");
        return NULL;
    }

    uint64_t cell_index;
    uint32_t bit_index;

    for (uint64_t i = 0; i < count; ++i, ++index)
    {
        cell_index = index / BITMAP_CELL_BIT_SIZE;
        bit_index = index % BITMAP_CELL_BIT_SIZE;

        bitmap[cell_index] = set_bit(bitmap[cell_index], bit_index);
    }
    
    return (void*)((index - count) * PMM_BLOCK_SIZE);
}

void pmm_free_blocks(void* start, uint32_t count) {
    uint64_t index = (uint64_t)start / PMM_BLOCK_SIZE;
    uint64_t cell_index;
    uint32_t bit_index;

    for (uint64_t i = 0; i < count; ++i, ++index)
    {
        cell_index = index / BITMAP_CELL_BIT_SIZE;
        bit_index = index % BITMAP_CELL_BIT_SIZE;

        bitmap[cell_index] = free_bit(bitmap[cell_index], bit_index);
    }
}

void* pmm_get_root_ptable() {
    return (void*)read_cr3();
}

void pmm_load_root_ptable(void* phys) {
    write_cr3((uint64_t)phys);
}
