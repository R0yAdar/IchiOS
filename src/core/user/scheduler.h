#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

void scheduler_add_process(process_ctx* ctx);

void scheduler_transfer_ctrl();

#endif