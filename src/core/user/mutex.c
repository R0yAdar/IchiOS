#include "mutex.h"

void spin_lock(spinlock_t *lock)
{
    asm volatile(
        "1: \n"
        "pause \n"
        "mov $1, %%eax \n"
        "xchg %%eax, %0 \n"
        "test %%eax, %%eax \n"
        "jnz 1b \n"
        : "+m"(lock->locked)
        :
        : "rax", "memory");
}

void spin_unlock(spinlock_t *lock)
{
    asm volatile(
        "movl $0, %0 \n"
        : "=m"(lock->locked)
        :
        : "memory");
}