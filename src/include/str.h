#include "types.h"

#ifndef STR_H
#define STR_H

int strlen(const char* text);

const char* int_to_str(unsigned long long value);

const char* int_to_hex(unsigned long long value);

int strcmp(const char *string1, const char *string2);

#endif