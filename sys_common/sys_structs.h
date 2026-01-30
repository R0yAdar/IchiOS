#ifndef SYS_STRUCTS_H
#define SYS_STRUCTS_H

#ifndef STDINT_H
typedef unsigned long long uint64_t;
#endif

typedef struct
{
    unsigned int x;
    unsigned int y;
    unsigned short color;
    char c;
    unsigned short scale;
} sys_put_c;

#define SYSCALL_FILE_OPS_CODE 0x6

typedef enum {
    SYS_FILE_OPEN = 0,
    SYS_FILE_CLOSE,
    SYS_FILE_READ,
    SYS_FILE_SEEK,
    SYS_FILE_TELL
} sys_file_action_id;

typedef struct
{
    uint64_t id;
    sys_file_action_id action;
    void* data;
    uint64_t data_len;
} sys_file_action;

#endif