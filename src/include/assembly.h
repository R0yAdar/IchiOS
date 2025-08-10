#include "stdint.h"

uint64_t read_cr3() {
    uint64_t value;
    
    __asm__ volatile (
        "mov %%cr3, %0"
        : "=r" (value)
    );
    
    return value;
}

void write_cr3(uint64_t value) {
    __asm__ volatile (
        "mov %0, %%cr3" 
        : : "r"(value) 
        : "memory");
}