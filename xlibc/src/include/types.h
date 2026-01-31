#ifndef TYPES_H
#define TYPES_H

#include "stdint.h"
#include "err.h"

typedef uint8_t BOOL;

#define FALSE (BOOL)0
#define TRUE (BOOL)1

#define DWORD_SIZE 4

#define PAGE_SIZE 4096
#define SECTOR_SIZE 512

#define NULL (void *)0

#endif
