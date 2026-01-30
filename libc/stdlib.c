#include "stdlib.h"
#include "string.h"

#define HEAP_START_ADDRESS 0x100000000
#define HEAP_SIZE (0xA00000)

uint64_t _heap_start = HEAP_START_ADDRESS;
uint64_t _heap_end = HEAP_START_ADDRESS + HEAP_SIZE;
uint64_t _heap_current = HEAP_START_ADDRESS;


void* malloc(size_t size) {
    if (_heap_current + size > _heap_end)
        return NULL;

    void* ptr = (void*)_heap_current;
    _heap_current += size;
    return ptr;
}

void free(void* ptr) {
    return;
}

void* calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void* ptr = malloc(total);
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

void* realloc(void* ptr, size_t size) {
    return NULL;
}

int atoi(const char* nptr) {
    int res = 0;
    int sign = 1;
    while (*nptr == ' ' || *nptr == '\t' || *nptr == '\n') nptr++;
    if (*nptr == '-') {
        sign = -1;
        nptr++;
    } else if (*nptr == '+') {
        nptr++;
    }
    while (*nptr >= '0' && *nptr <= '9') {
        res = res * 10 + (*nptr - '0');
        nptr++;
    }
    return res * sign;
}

float atof(const char* nptr) {
    return 0.0;
}

int abs(int j) {
    return (j < 0) ? -j : j;
}

int system(const char* command) {
    return 0;
}

void exit(int status) {
    return;
}