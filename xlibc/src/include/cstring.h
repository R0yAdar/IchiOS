#ifndef CSTRING_H
#define CSTRING_H

#include "stdint.h"
#include "stdarg.h"
#include "types.h"

#define LENGTH(arr) sizeof(arr) / sizeof(arr[0])

void* memset(void* dest, uint8_t value, uint64_t count);

int number_as_string(unsigned long long value, char* buffer, int base);

int vsprintf(char* s, const char* format, va_list arg);

#endif