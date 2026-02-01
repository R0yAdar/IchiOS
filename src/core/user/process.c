#include "process.h"
#include "vmm.h"
#include "elf.h"
#include "serial.h"
#include "assembly.h"
#include "scheduler.h"

#define STACK_ADDRESS 0x150000000
#define STACK_SIZE (4096 * 1024)
#define HEAP_ADDRESS 0x100000000
#define HEAP_SIZE (0xF00000)
#define CS_PL3 0x1B
#define SS_PL3 0x23
#define RFLAGS_INIT 0x202

uint64_t _pid = 0;

uint64_t get_pid()
{
    return _pid++;
}

struct process_ctx
{
    uint64_t pid;

    pagetable_context *vmem_ctx;
    stack_layout exec_ctx;
    process_state state;
};

process_ctx *process_create()
{
    process_ctx *ctx = kmalloc(sizeof(process_ctx));

    if (!ctx)
        return NULL;

    ctx->vmem_ctx = vmm_create_userspace_context();
    ctx->pid = get_pid();
    ctx->state = PROCESS_CREATED;

    ctx->exec_ctx.cs = CS_PL3;
    ctx->exec_ctx.ss = SS_PL3;
    ctx->exec_ctx.rflags = RFLAGS_INIT;

    qemu_logf("Process created with pid %d", ctx->pid);

    return ctx;
}

void process_exec(process_ctx *ctx, file *elf)
{
    ELF_ERRORS error;
    elf_info info;

    elf_context *elf_ctx = elf_open(elf, &error);
    elf_get_layout(elf_ctx, &info);

    if (!vmm_allocate_umm(ctx->vmem_ctx, info.vaddr, info.size))
    {
        qemu_log("Failed to allocate memory");
        elf_release(elf_ctx);
        return;
    }

    vmm_apply_pagetable(ctx->vmem_ctx);

    qemu_logf("Loading ELF at %x", info.vaddr);

    void *entry;
    error = elf_load_into(elf_ctx, (void *)info.vaddr, &entry);

    elf_release(elf_ctx);

    if (error != ELF_NO_ERROR)
    {
        qemu_log("Failed to load ELF");
        return;
    }

    if (!vmm_allocate_umm(ctx->vmem_ctx, STACK_ADDRESS, STACK_SIZE))
    {
        qemu_log("Failed to allocate stack");
        return;
    }

    if (!vmm_allocate_umm(ctx->vmem_ctx, HEAP_ADDRESS, HEAP_SIZE))
    {
        qemu_log("Failed to allocate heap");
        return;
    }

    ctx->exec_ctx.rsp = STACK_ADDRESS + STACK_SIZE;
    ctx->exec_ctx.rip = (uint64_t)entry;
    ctx->state = PROCESS_READY;
}

void process_init_idle(process_ctx *ctx)
{
    vmm_apply_pagetable(ctx->vmem_ctx);

    vmm_allocate_umm(ctx->vmem_ctx, 0x1000, 4096);
    (*(uint8_t *)0x1000) = 0xEB;
    (*(uint8_t *)0x1001) = 0xFE;

    ctx->exec_ctx.rip = 0x1000;

    vmm_allocate_umm(ctx->vmem_ctx, 0x10000, 4096);
    ctx->exec_ctx.rsp = 0x10000;

    ctx->state = PROCESS_IDLE;
}

void process_stop(process_ctx *ctx, volatile stack_layout *stack)
{
    if (ctx->state != PROCESS_BLOCKED && ctx->state != PROCESS_IDLE)
    {
        ctx->state = PROCESS_READY;
    }

    ctx->exec_ctx = *stack;
}

extern void processUserlandTrampoline(stack_layout *ctx) __attribute__((noreturn));

__attribute__((noreturn)) void process_resume(process_ctx *ctx)
{
    if (ctx->state != PROCESS_IDLE)
    {
        ctx->state = PROCESS_READY;
    }

    vmm_apply_pagetable(ctx->vmem_ctx);

    processUserlandTrampoline(&ctx->exec_ctx);
}

void process_block(process_ctx *ctx)
{
    ctx->state = PROCESS_BLOCKED;
}

void process_unblock(process_ctx *ctx)
{
    ctx->state = PROCESS_READY;
}

process_state process_get_state(process_ctx *ctx)
{
    return ctx->state;
}

uint64_t process_get_pid(process_ctx *ctx)
{
    return ctx->pid;
}

void process_exit(process_ctx *ctx)
{
    vmm_destroy_userspace_context(ctx->vmem_ctx);
    kfree(ctx);
}