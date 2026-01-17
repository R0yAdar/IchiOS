#include "process.h"
#include "vmm.h"
#include "elf.h"
#include "serial.h"

typedef struct
{
    void* stack;
    void* entry;
} execution_context;


typedef struct {
    uint64_t pid;
    pagetable_context* vmem_ctx;
    void* lea; // Last Execution Address
} process_ctx;

process_ctx* process_create() {
    process_ctx* ctx = kmalloc(sizeof(process_ctx));
    if (!ctx) return NULL;

    ctx->vmem_ctx = vmm_create_userspace_context();
    ctx->pid = 0;

    qemu_logf("Process created with pid %d", ctx->pid);

    return ctx;
}

void process_exec(process_ctx* ctx, file* elf) {
    elf_load(elf);
}