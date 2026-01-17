#include "process.h"
#include "vmm.h"
#include "elf.h"
#include "serial.h"
#include "assembly.h"

#define STACK_ADDRESS 0x70000000
#define STACK_SIZE (4096 * 2)

typedef struct
{
    void* stack;
    void* entry;
} execution_context;


struct process_ctx {
    uint64_t pid;
    pagetable_context* vmem_ctx;
    void* lea;
};

process_ctx* process_create() {
    process_ctx* ctx = kmalloc(sizeof(process_ctx));
    if (!ctx) return NULL;

    ctx->vmem_ctx = vmm_create_userspace_context();
    ctx->pid = 0;

    qemu_logf("Process created with pid %d", ctx->pid);

    return ctx;
}

void process_exec(process_ctx* ctx, file* elf) {
    ELF_ERRORS error;
    elf_info info;

    elf_context* elf_ctx =  elf_open(elf, &error);
    elf_get_layout(elf_ctx, &info);

    if (!vmm_allocate_umm(ctx->vmem_ctx, info.vaddr, info.size)) {
        qemu_log("Failed to allocate memory");
        elf_release(elf_ctx);
        return;
    }

    vmm_apply_pagetable(ctx->vmem_ctx);

    qemu_logf("Loading ELF at %x", info.vaddr);

    void* entry;    
    error = elf_load_into(elf_ctx, (void*)info.vaddr, &entry);
    
    elf_release(elf_ctx);

    if (error != ELF_NO_ERROR) {
        qemu_log("Failed to load ELF");
        return;
    }

    if (!vmm_allocate_umm(ctx->vmem_ctx, STACK_ADDRESS, STACK_SIZE)) {
        qemu_log("Failed to allocate stack");
        return;
    }

    qemu_logf("Jumping to userland at %x", entry);
    jump_to_userland(STACK_ADDRESS + STACK_SIZE, (uint64_t)entry);
}