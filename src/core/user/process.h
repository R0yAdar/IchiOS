#ifndef PROCESS_H
#define PROCESS_H

#include "vfs.h"

typedef struct process_ctx process_ctx;

process_ctx* process_create();

void process_exec(process_ctx* ctx, file* elf);

#endif