#ifndef CSTRING_H
#define CSTRING_H

#include "stdint.h"
#include "stdarg.h"
#include "types.h"

#define LENGTH(arr) sizeof(arr) / sizeof(arr[0])

int memcmp(const uint8_t* left, const uint8_t* right, size_t len);

void* memcpy(void* dest, void* src, uint64_t count);

void* memset(void* dest, uint8_t value, uint64_t count);

int number_as_string(unsigned long long value, char* buffer, int base);

int vsprintf(char* s, const char* format, va_list arg);

#endif