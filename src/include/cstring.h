#ifndef CSTRING_H
#define CSTRING_H

#include "stdint.h"
#include "stdarg.h"

#define LENGTH(arr) sizeof(arr) / sizeof(arr[0])
#define NULL (void*)0

void* memset(void* dest, uint8_t value, uint64_t count);

int vsprintf(char* s, const char* format, va_list arg);

#endif