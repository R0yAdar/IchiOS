#include "sys_structs.h"

#define NULL (void*)0

typedef unsigned long long uint64_t;


void syscall(uint64_t id, void* ptr) {
    asm volatile( "int $0x80" :: "a"(id), "c"(ptr) : "memory" );
}
