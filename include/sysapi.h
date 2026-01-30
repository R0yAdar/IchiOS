#include "sys_structs.h"

#define NULL (void*)0

void syscall(uint64_t id, void* ptr) {
    __asm__ __volatile__( "int $0x80" :: "a"(id), "c"(ptr) : "memory" );
}

void syscall_echo(char* msg) {
    syscall(2, msg);
}

void syscall_putc(char c) {
    syscall(4, c);
}

void syscall_file_open(char* filename, uint64_t flags) {
    sys_file_action data = {
        .id = 0,
        .action = SYS_FILE_OPEN,
        .data = filename,
    };

    syscall(SYSCALL_FILE_OPS_CODE, &data);
}

uint64_t syscall_file_write(uint64_t fid, void* buffer, uint64_t buffer_len) {
    sys_file_action data = {
        .id = fid,
        .action = SYS_FILE_WRITE,
        .data = buffer,
        .data_len = buffer_len,
    };

    syscall(SYSCALL_FILE_OPS_CODE, &data);

    return data.data_len;
}

uint64_t syscall_file_read(uint64_t fid, void* buffer, uint64_t buffer_len) {
    sys_file_action data = {
        .id = fid,
        .action = SYS_FILE_WRITE,
        .data = buffer,
        .data_len = buffer_len,
    };

    syscall(SYSCALL_FILE_OPS_CODE, &data);

    return data.data_len;
}