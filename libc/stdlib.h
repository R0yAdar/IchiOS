#ifndef STDLIB_H
#define STDLIB_H

#include "stdint.h"
#include "stdio.h"

void *malloc(size_t size);

void free(void *ptr);

void *calloc(size_t nmemb, size_t size);

void *realloc(void *ptr, size_t size);

int atoi(const char *nptr);

float atof(const char *nptr);

int abs(int j);

int system(const char *command);

void exit(int status);

#endif