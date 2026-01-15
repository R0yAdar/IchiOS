#include "elf.h"
#include "vmm.h"
#include "serial.h"
#include "assembly.h"

typedef struct {
    void* data;
    void* entry;
} elf;

#pragma pack (push, 1)

typedef struct {
    uint8_t  ident[16]; uint16_t type; uint16_t machine; uint32_t ver;
    uint64_t entry; uint64_t phoff; uint64_t shoff; uint32_t flags;
    uint16_t ehsize; uint16_t phentsize; uint16_t phnum;
} Ehdr;

typedef struct {
    uint32_t type; uint32_t flags; uint64_t offset; uint64_t vaddr;
    uint64_t paddr; uint64_t filesz; uint64_t memsz; uint64_t align;
} Phdr;

#pragma pack (pop)

uint8_t ELF_MAGIC[4] = { 0x7F, 0x45, 0x4C, 0x46 };

void load_and_jump(void* elf_raw) {
    pagetable_context* ctx = vmm_create_userspace_context();
    vmm_apply_pagetable(ctx);

    Ehdr* ehdr = (Ehdr*)elf_raw;
    Phdr* phdr = (Phdr*)((uint8_t*)elf_raw + ehdr->phoff);

    if (ehdr->ident[0] != ELF_MAGIC[0] || ehdr->ident[1] != ELF_MAGIC[1] || ehdr->ident[2] != ELF_MAGIC[2] || ehdr->ident[3] != ELF_MAGIC[3]) {
        qemu_log("ELF: Invalid magic");
        return;
    }

    qemu_logf("Header count is %d", ehdr->phnum);

    for (int i = 0; i < ehdr->phnum; i++) {
        qemu_logf("Processing program header %d", phdr[i].type);
        if (phdr[i].type == 1) { // PT_LOAD = 1
            // Copy data from the file buffer to the required Virtual Address
            unsigned char* dest = (unsigned char*)phdr[i].vaddr;
            unsigned char* src  = (unsigned char*)elf_raw + phdr[i].offset;

            qemu_logf("Copying %d bytes from %x to %x", phdr[i].filesz, src, dest);
            
            vmm_allocate_umm(ctx, (uint64_t)dest, 4096);
            
            for (uint64_t j = 0; j < phdr[i].filesz; j++) dest[j] = src[j];
            
            // Zero out the BSS (memory size > file size)
            for (uint64_t j = phdr[i].filesz; j < phdr[i].memsz; j++) dest[j] = 0;
        }
    }
    qemu_logf("Jumping to program entry point %x", ehdr->entry);

    cli();

    uint64_t stack = 0x700000;

    vmm_allocate_umm(ctx, stack, 4096 * 2);

    qemu_log("===========START==========");
    qemu_dump((void*)ehdr->entry, 200);
    qemu_log("===========END==========");


    qemu_log("Jumping to userland");
    jump_to_userland(stack + 4096 + 2000, ehdr->entry);
}

void elf_load(file* f) {
    qemu_logf("ELF: Loading ELF file of size %d...", f->size);
    void* buffer = kmalloc(f->size);

    if (!buffer) {
        qemu_log("ELF: Failed to allocate buffer");
        return;
    }

    size_t read = fread(buffer, 1, f->size, f);
    if ((uint64_t)read != f->size) {
        qemu_logf("ELF: Failed to read file (%d/%d)", read, f->size);
        return;
    }

    qemu_log("Dumping ELF file");

    qemu_dump(buffer, f->size);
    
    load_and_jump(buffer);
}


typedef struct {
    uint16_t something;
} elf_context;

typedef enum {
    ELF_NO_ERROR = 0,
    ELF_INVALID_FILE,
    ELF_INVALID_HEADER,
    ELF_INVALID_PROGRAM_HEADER,
    ELF_INVALID_SECTION_HEADER,
    ELF_INVALID_SYMBOL_TABLE,
    ELF_INVALID_STRING_TABLE,
    ELF_UNKNOWN_ERROR
} ELF_ERROR_CODE;

/*
elf_context* elf_init(void* elf_raw) {

}

uint64_t elf_compute_memory_requirements() {

}

ELF_ERROR_CODE elf_load(elf_context* ctx, void* target) {
}

void elf_destroy(elf_context* ctx) {
}
*/