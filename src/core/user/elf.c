#include "elf.h"
#include "vmm.h"
#include "serial.h"

typedef struct {
    void* data;
    void* entry;
} elf;

typedef struct {
    uint8_t  ident[16]; uint16_t type; uint16_t machine; uint32_t ver;
    uint64_t entry; uint64_t phoff; uint64_t shoff; uint32_t flags;
    uint16_t ehsize; uint16_t phentsize; uint16_t phnum;
} Ehdr;

typedef struct {
    uint32_t type; uint32_t flags; uint64_t offset; uint64_t vaddr;
    uint64_t paddr; uint64_t filesz; uint64_t memsz; uint64_t align;
} Phdr;

void load_and_jump(void* elf_raw) {
    Ehdr* ehdr = (Ehdr*)elf_raw;
    Phdr* phdr = (Phdr*)((uint8_t*)elf_raw + ehdr->phoff);

    for (int i = 0; i < ehdr->phnum; i++) {
        if (phdr[i].type == 1) { // PT_LOAD = 1
            // Copy data from the file buffer to the required Virtual Address
            unsigned char* dest = (unsigned char*)phdr[i].vaddr;
            unsigned char* src  = (unsigned char*)elf_raw + phdr[i].offset;

            qemu_logf("Copying %d bytes from %d to %d", phdr[i].filesz, src, dest);
            allocate_umm((uint64_t)dest, 4096);
            
            for (uint64_t j = 0; j < phdr[i].filesz; j++) dest[j] = src[j];
            
            // Zero out the BSS (memory size > file size)
            for (uint64_t j = phdr[i].filesz; j < phdr[i].memsz; j++) dest[j] = 0;
        }
    }

    // Jump to the entry point
    ((void (*)())ehdr->entry)();
}

void elf_load(file* f) {
    qemu_logf("ELF: Loading ELF file of size %d...", f->size);
    void* buffer = kmalloc(f->size);

    if (!buffer) {
        qemu_log("ELF: Failed to allocate buffer");
        return;
    }

    if (!fread(buffer, 1, f->size, f)) {
        qemu_log("ELF: Failed to read file");
        return;
    }
    
    load_and_jump(buffer);
}