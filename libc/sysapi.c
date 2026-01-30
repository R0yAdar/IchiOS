#include "sysapi.h"
#include "../../sys_common/sys_structs.h"

void syscall(uint64_t id, void* ptr) {
    __asm__ __volatile__( "int $0x80" :: "a"(id), "c"(ptr) : "memory" );
}

void syscall_sleep(uint64_t ms) {
    syscall(1, ms);
}

void syscall_echo(char* msg) {
    syscall(2, msg);
}

void syscall_putc(char c) {
    syscall(4, c);
}

uint64_t syscall_get_uptime() {
    uint64_t result;
    syscall(5, &result);

    return result;
}

void syscall_file_open(char* filename, uint64_t* out_fid) {
    sys_file_action data = {
        .id = 0,
        .action = SYS_FILE_OPEN,
        .data = filename,
    };

    syscall(SYSCALL_FILE_OPS_CODE, &data);
    *out_fid = data.id;
}

uint64_t syscall_file_read(uint64_t fid, void* buffer, uint64_t buffer_len) {
    sys_file_action data = {
        .id = fid,
        .action = SYS_FILE_READ,
        .data = buffer,
        .data_len = buffer_len,
    };

    syscall(SYSCALL_FILE_OPS_CODE, &data);

    return data.data_len;
}

uint64_t syscall_file_seek(uint64_t fid, uint64_t offset, int whence) {
}

uint64_t syscall_file_tell(uint64_t fid) {
    return 0;
}

void syscall_file_close(uint64_t fid) {
}