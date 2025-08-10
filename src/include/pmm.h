#ifndef PMM_H
#define PMM_H

#include "stdint.h"
#include "multiboot.h"

#define PMM_BLOCK_SIZE 4096
#define RESERVED_KERNEL_MEMORY_BYTES 32768 // 32 (KB)

typedef enum pmm_strategy_t {
    BITMAP
} pmm_strategy;

typedef struct pmm_context_t {
    uint32_t low_memory;
    uint32_t high_memory;
    memory_region* regions;
    uint32_t regions_count;
    pmm_strategy strategy;
    uint32_t kernel_ram_size;
} pmm_context;
    
int     pmm_init(pmm_context* context);

void*   pmm_alloc();

void    pmm_free(void* page);

void*   pmm_alloc_blocks(uint32_t count);

void    pmm_free_blocks(void* start, uint32_t count);

void*   pmm_get_pdbr();

void    pmm_load_pdbr(void* phys);

#endif