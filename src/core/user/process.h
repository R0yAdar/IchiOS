#ifndef PROCESS_H
#define PROCESS_H

#include "vfs.h"

typedef struct process_ctx process_ctx;

process_ctx* process_create();

void process_exec(process_ctx* ctx, file* elf);

void process_stop(process_ctx *ctx, uint64_t stack_ptr, uint64_t instruction_ptr);

void process_resume(process_ctx *ctx);

void process_exit(process_ctx *ctx);

#endif