#include "types.h"

#ifndef STR_H
#define STR_H

int strlen(const char *text);

const char *int_to_str(unsigned long long value);

const char *int_to_hex(unsigned long long value);

int strcmp(const char *string1, const char *string2);

const char *strstr(const char *str1, const char *str2);

const char *strchr(const char *str, char character);

int strncmp(const char *str1, const char *str2, size_t num);

char *strcpy(char *destination, const char *source);

#endif