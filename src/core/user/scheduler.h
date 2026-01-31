#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

BOOL scheduler_init();

void scheduler_add_process(process_ctx* ctx);

void scheduler_transfer_ctrl();

process_ctx* scheduler_get_active();

#endif