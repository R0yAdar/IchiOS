#ifndef SPINLOCK_H
#define SPINLOCK_H

#include "stdint.h"

typedef struct
{
    volatile uint32_t locked;
} spinlock_t;

#define SPINLOCK_INIT {0}

void spin_lock(spinlock_t *lock);

void spin_unlock(spinlock_t *lock);

#endif