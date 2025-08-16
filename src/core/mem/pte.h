#ifndef PTE_H
#define PTE_H

#include "stdint.h"

typedef uint64_t pte_t;
typedef uint8_t pte_flags_t;

#define PAGE_SIZE 4096
#define ZERO_PTE (pte_t)0

enum x64_PTE {
    x64_PTE_FLAG_PRESENT = 1,
    x64_PTE_FLAG_WRITEABLE = 2,
    x64_PTE_FLAG_USER_ACCESSIBLE = 4,
    x64_PTE_FLAG_WRITE_THROUGH_CACHING = 8,
    x64_PTE_FLAG_DISABLE_CACHE = 16,
    x64_PTE_FLAG_ACCESSED = 32,
    x64_PTE_FLAG_DIRTY = 64,
    x64_PTE_FLAG_HUGE_PAGE = 128,
    x64_PTE_FLAG_GLOBAL = 256,

    x64_PTE_MASK_AVAILABLE_1 = 512 + 1024 + 2048,
    x64_PTE_MASK_PHYSICAL_ADDRESS = 0xFFFFFFFFFF000,
    x64_PTE_MASK_AVAILABLE_2 = 0x7FF0000000000000,

    x64_PTE_FLAG_NO_EXECUTE = 0x8000000000000000
};

#define DEFAULT_PTE (pte_t)(1 + 2)

// address has to be 4096 byte aligned
void assign_address(pte_t* pte, void* address) {
    *pte &= (~x64_PTE_MASK_PHYSICAL_ADDRESS);
    *pte |= ((uint64_t)address & x64_PTE_MASK_PHYSICAL_ADDRESS);
}

void mark_present(pte_t* pte) {
    *pte |= x64_PTE_FLAG_PRESENT;
}

void mark_writeable(pte_t* pte) {
    *pte |= x64_PTE_FLAG_WRITEABLE;
}

#endif