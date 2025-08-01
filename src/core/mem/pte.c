#include "stdint.h"

typedef uint64_t x64_pte;

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

typedef struct pte_specs_t {
    uint8_t flags;

    // only the first 40 bits count. bits 0-11 are always zero (limited to 52 bit physical addresses)
    uint64_t physical_address;

} pte_specs;

