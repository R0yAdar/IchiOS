#include "array.h"

uint64_t copy(void* src, void* dst, uint64_t len) {
    for (uint64_t i = 0; i < len; i++) {
        ((uint8_t*)dst)[i] = ((uint8_t*)src)[i];
    }

    return len;
}

uint64_t copy_to(void* src, void* dst, uint64_t dest_start, uint64_t len) {
    for (uint64_t i = 0; i < len; i++) {
        ((uint8_t*)dst)[dest_start + i] = ((uint8_t*)src)[i];
    }

    return len;
}