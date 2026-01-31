#ifndef ARRAY_H
#define ARRAY_H

#include "types.h"

uint64_t copy(void *src, void *dst, uint64_t len);

uint64_t copy_to(void *src, void *dst, uint64_t dest_start, uint64_t len);

#endif