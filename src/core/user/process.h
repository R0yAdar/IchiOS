#ifndef PROCESS_H
#define PROCESS_H

#include "types.h"

typedef struct process_ctx {
    uint64_t id;
    void* vmem_ctx;
    void* lea; // Last Execution Address
};

void process_create(process_ctx* out_ctx, uint64_t out_id);

void process_run(process_ctx* ctx);


#endif