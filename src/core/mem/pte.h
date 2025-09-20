#ifndef PTE_H
#define PTE_H

#include "stdint.h"

typedef uint64_t pte_t;
typedef uint8_t pte_flags_t;

#define PAGE_SIZE 4096
#define ZERO_PTE (pte_t)0
#define x64_PTE_FLAG_PRESENT 1
#define x64_PTE_FLAG_WRITEABLE 2
#define x64_PTE_FLAG_USER_ACCESSIBLE 4
#define x64_PTE_FLAG_WRITE_THROUGH_CACHING 8
#define x64_PTE_FLAG_DISABLE_CACHE 16
#define x64_PTE_FLAG_ACCESSED 32
#define x64_PTE_FLAG_DIRTY 64
#define x64_PTE_FLAG_HUGE_PAGE 128
#define x64_PTE_FLAG_GLOBAL 256
#define x64_PTE_MASK_AVAILABLE_1 512 + 1024 + 2048
#define x64_PTE_MASK_PHYSICAL_ADDRESS 0xFFFFFFFFFF000ull
#define x64_PTE_MASK_AVAILABLE_2 0x7FF0000000000000ull
#define x64_PTE_FLAG_NO_EXECUTE 0x8000000000000000ull

#define DEFAULT_PTE (pte_t)(x64_PTE_FLAG_PRESENT + x64_PTE_FLAG_WRITEABLE)
#define DEFAULT_HUGE_PTE (pte_t)(x64_PTE_FLAG_PRESENT + x64_PTE_FLAG_WRITEABLE + x64_PTE_FLAG_HUGE_PAGE)
#define PDPT_HUGE_PAGE_SIZE 1024 * 1024 * 1024

// address has to be 4096 byte aligned
void assign_address(pte_t* pte, void* address) {
    *pte &= (~x64_PTE_MASK_PHYSICAL_ADDRESS);
    *pte |= ((uint64_t)address & x64_PTE_MASK_PHYSICAL_ADDRESS);
}

void* pte_get_address(pte_t* pte) {
    return (void*)(*pte & x64_PTE_MASK_PHYSICAL_ADDRESS);
}

void mark_present(pte_t* pte) {
    *pte |= x64_PTE_FLAG_PRESENT;
}

void mark_writeable(pte_t* pte) {
    *pte |= x64_PTE_FLAG_WRITEABLE;
}

void mark_user_space(pte_t* pte) {
    *pte |= x64_PTE_FLAG_USER_ACCESSIBLE;
}

#endif