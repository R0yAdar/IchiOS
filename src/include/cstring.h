#ifndef CSTRING_H
#define CSTRING_H

#include "stdint.h"

#define LENGTH(arr) sizeof(arr) / sizeof(arr[0])
#define NULL 0

void* memset(void* dest, uint8_t value, uint64_t count);

#endif