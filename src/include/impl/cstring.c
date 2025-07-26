#include "cstring.h"

// can remove i to optimize
void* memset(void* dest, uint8_t value, uint64_t count) {
    for(uint64_t i = 0; i < count; ++i) {
        *((char*)(dest) + i) = value;
    }

    return dest;
}