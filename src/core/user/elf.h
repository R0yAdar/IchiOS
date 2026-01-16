#ifndef ELF_H
#define ELF_H

#include "vfs.h"

typedef struct elf elf;

void elf_load(file* f);

#endif