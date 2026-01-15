#ifndef PROCESS_H
#define PROCESS_H

#include "types.h"
#include "vmm.h"

typedef struct process_ctx {
    uint64_t id;
    pagetable_context* vmem_ctx;
    void* lea; // Last Execution Address
};

process_ctx* process_create(pagetable_context* vmem_ctx);

void process_run(process_ctx* ctx);


#endif