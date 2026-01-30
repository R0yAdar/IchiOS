#include "stdlib.h"

#include "stdlib.h"
#include "string.h"

void* malloc(size_t size) {
    // syscall: sbrk or mmap to request memory from the kernel
    return NULL;
}

void free(void* ptr) {
    // syscall: depends on your allocator logic, usually marks block as free
    return;
}

void* calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void* ptr = malloc(total);
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

void* realloc(void* ptr, size_t size) {
    if (!ptr) return malloc(size);
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    void* new_ptr = malloc(size);
    if (new_ptr) {
        // Note: This is inefficient without knowing the original size, 
        // but works for a minimal libc interface.
        memcpy(new_ptr, ptr, size); 
        free(ptr);
    }
    return new_ptr;
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