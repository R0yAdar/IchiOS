#ifndef PROCESS_H
#define PROCESS_H

#include "vfs.h"

typedef struct process_ctx process_ctx;

#pragma pack(push, 1)

typedef struct
{
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rax;

    uint64_t rbp;
    uint64_t rbx;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;

    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} stack_layout;

#pragma pack(pop)

typedef enum
{
    PROCESS_CREATED,
    PROCESS_READY,
    PROCESS_ACTIVE,
    PROCESS_IDLE,
    PROCESS_BLOCKED
} process_state;

process_ctx *process_create();

void process_exec(process_ctx *ctx, file *elf);

void process_init_idle(process_ctx *ctx);

void process_stop(process_ctx *ctx, volatile stack_layout *stack);

void process_resume(process_ctx *ctx);

void process_block(process_ctx *ctx);

void process_unblock(process_ctx *ctx);

process_state process_get_state(process_ctx *ctx);

uint64_t process_get_pid(process_ctx *ctx);

void process_exit(process_ctx *ctx);

#endif