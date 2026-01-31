#ifndef SYSAPI_H
#define SYSAPI_H

#include "../../sys_common/sys_structs.h"

#define NULL (void*)0

void syscall(uint64_t id, void* ptr);

void syscall_sleep(uint64_t ms);

void syscall_echo(char* msg);

void syscall_puts(char* str);

uint64_t syscall_get_uptime();

void syscall_file_open(char* filename, uint64_t* out_fid);

uint64_t syscall_file_read(uint64_t fid, void* buffer, uint64_t buffer_len);

uint64_t syscall_file_seek(uint64_t fid, uint64_t offset, int whence);

uint64_t syscall_file_tell(uint64_t fid);

void syscall_file_close(uint64_t fid);

void syscall_draw_window(uint32_t width, uint32_t height, uint32_t* buffer);

void syscall_get_key(uint32_t* last_key, uint32_t* was_pressed);

#endif