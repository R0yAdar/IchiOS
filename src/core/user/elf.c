#include "elf.h"
#include "vmm.h"
#include "serial.h"
#include "assembly.h"
#include "cstring.h"

uint8_t ELF_MAGIC[4] = {0x7F, 0x45, 0x4C, 0x46};

#define PHDR_TYPE_PT_LOAD 1
#define PHDR_TYPE_PT_DYNAMIC 2
#define EHDR_TYPE_REL 1
#define EHDR_TYPE_EXEC 2
#define EHDR_TYPE_DYN 3
#define DT_NULL 0
#define DT_RELA 7
#define DT_RELASZ 8
#define R_X86_64_RELATIVE 8
#define ELF64_R_TYPE(i) ((i) & 0xFFFFFFFFL)

typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef int32_t Elf64_Sword;
typedef uint64_t Elf64_Xword;
typedef int64_t Elf64_Sxword;
typedef uint8_t Elf64_UnsignedChar;

#pragma pack(push, 1)

typedef struct
{
    uint8_t ident[16];
    uint16_t type;
    uint16_t machine;
    uint32_t ver;
    uint64_t entry;
    uint64_t phoff;
    uint64_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
} Ehdr;

typedef struct
{
    uint32_t type;
    uint32_t flags;
    uint64_t offset;
    uint64_t vaddr;
    uint64_t paddr;
    uint64_t filesz;
    uint64_t memsz;
    uint64_t align;
} Phdr;

#pragma pack(pop)

struct elf_context
{
    void *elf_raw;
    Phdr *phdrs;
    Ehdr *ehdr;
    elf_info info;
};

typedef struct
{
    Elf64_Sxword d_tag;
    union
    {
        Elf64_Xword d_val;
        Elf64_Addr d_ptr;
    } d_un;
} Elf64_Dyn;

typedef struct
{
    Elf64_Addr r_offset;
    Elf64_Xword r_info;
    Elf64_Sxword r_addend;
} Elf64_Rela;

elf_context *elf_open(file *elf_file, ELF_ERRORS *out_error)
{
    void *buffer = kmalloc(elf_file->size);

    if ((uint64_t)fread(buffer, 1, elf_file->size, elf_file) != elf_file->size)
    {
        kfree(buffer);
        return NULL;
    }

    elf_context *ctx = kmalloc(sizeof(elf_context));

    ctx->elf_raw = buffer;
    ctx->ehdr = (Ehdr *)ctx->elf_raw;
    ctx->phdrs = (Phdr *)((uint8_t *)ctx->elf_raw + ctx->ehdr->phoff);

    if (memcmp(ctx->ehdr->ident, ELF_MAGIC, sizeof(ELF_MAGIC)) != 0)
    {
        *out_error = ELF_INVALID_MAGIC;
        elf_release(ctx);
        return NULL;
    }

    if (ctx->ehdr->type != EHDR_TYPE_EXEC && ctx->ehdr->type != EHDR_TYPE_DYN)
    {
        *out_error = ELF_UNSUPPORTED;
        elf_release(ctx);
        return NULL;
    }

    return ctx;
}

void elf_get_layout(elf_context *ctx, elf_info *out_info)
{
    uint64_t min_v = (uint64_t)-1;
    uint64_t max_v = 0;
    uint64_t align = 4096;

    for (uint16_t i = 0; i < ctx->ehdr->phnum; i++)
    {
        if (ctx->phdrs[i].type == PHDR_TYPE_PT_LOAD)
        {
            if (ctx->phdrs[i].vaddr < min_v)
            {
                min_v = ctx->phdrs[i].vaddr;
            }
            if (ctx->phdrs[i].vaddr + ctx->phdrs[i].memsz > max_v)
            {
                max_v = ctx->phdrs[i].vaddr + ctx->phdrs[i].memsz;
            }
            if (ctx->phdrs[i].align > align)
            {
                align = ctx->phdrs[i].align;
            }
        }
    }

    *out_info = (elf_info){.vaddr = min_v, .size = max_v - min_v, .align = align, .dynamic = ctx->ehdr->type == EHDR_TYPE_DYN};
    ctx->info = *out_info;
}

BOOL elf_relocate(elf_context *ctx, void *base, Elf64_Dyn *relocation_table)
{
    Elf64_Rela *rela = 0;
    size_t sz = 0;

    for (Elf64_Dyn *entry = relocation_table; entry->d_tag != DT_NULL; entry++)
    {
        if (entry->d_tag == DT_RELA)
        {
            rela = (Elf64_Rela *)((uint64_t)base - ctx->info.vaddr + entry->d_un.d_ptr);
        }
        if (entry->d_tag == DT_RELASZ)
        {
            sz = entry->d_un.d_val;
        }
    }

    if (!rela)
    {
        return TRUE;
    }

    size_t count = sz / sizeof(Elf64_Rela);

    for (size_t i = 0; i < count; i++)
    {
        if (ELF64_R_TYPE(rela[i].r_info) == R_X86_64_RELATIVE)
        {
            uint64_t *target = (uint64_t *)((uint64_t)base + rela[i].r_offset);
            *target = (uint64_t)base + rela[i].r_addend;
        }
    }

    return TRUE;
}

ELF_ERRORS elf_load_into(elf_context *ctx, void *dest, void **out_entry)
{
    if (dest == NULL || ((uint64_t)dest != ctx->info.vaddr && ctx->info.dynamic == FALSE))
    {
        return ELF_INVALID_DESTINATION;
    }

    Elf64_Dyn *relocation_table = NULL;

    for (uint32_t i = 0; i < ctx->ehdr->phnum; i++)
    {
        Phdr *p = &ctx->phdrs[i];

        if (p->type == PHDR_TYPE_PT_LOAD)
        {
            void *seg_dest = (void *)((uint8_t *)dest - ctx->info.vaddr + p->vaddr);

            memcpy(seg_dest, (uint8_t *)ctx->elf_raw + p->offset, p->filesz);

            if (p->memsz > p->filesz)
            {
                memset((uint8_t *)seg_dest + p->filesz, 0, p->memsz - p->filesz);
            }
        }
        else if (p->type == PHDR_TYPE_PT_DYNAMIC)
        {
            relocation_table = (Elf64_Dyn *)((uint64_t)dest - ctx->info.vaddr + p->vaddr);
        }
    }

    if (relocation_table)
    {
        if (!elf_relocate(ctx, dest, relocation_table))
        {
            return ELF_INVALID_RELOCATION;
        }
    }

    *out_entry = (void *)((uint64_t)dest - ctx->info.vaddr + ctx->ehdr->entry);

    return ELF_NO_ERROR;
}

void elf_release(elf_context *ctx)
{
    kfree(ctx->elf_raw);
    kfree(ctx);
}
