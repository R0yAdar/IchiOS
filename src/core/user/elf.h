#ifndef ELF_H
#define ELF_H

#include "vfs.h"

typedef enum
{
    ELF_NO_ERROR = 0,
    ELF_INVALID_MAGIC,
    ELF_UNSUPPORTED,
    ELF_INVALID_DESTINATION,
    ELF_INVALID_RELOCATION
} ELF_ERRORS;

typedef struct
{
    uint64_t vaddr;
    uint64_t size;
    uint64_t align;
    BOOL dynamic;
} elf_info;

typedef struct elf_context elf_context;

elf_context *elf_open(file *elf_file, ELF_ERRORS *out_error);

void elf_get_layout(elf_context *ctx, elf_info *out_info);

ELF_ERRORS elf_load_into(elf_context *ctx, void *dest, void** out_entry);

void elf_release(elf_context *ctx);

#endif