#ifndef SYS_STRUCTS_H
#define SYS_STRUCTS_H

#ifndef STDINT_H
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
#endif

#define SYSCALL_HELLO 0x0
#define SYSCALL_SLEEP 0x1
#define SYSCALL_ECHO 0x2
#define SYSCALL_DRAW_CHAR 0x3
#define SYSCALL_PUTS 0x4
#define SYSCALL_GET_UPTIME 0x5
#define SYSCALL_FILE_OPS 0x6
#define SYSCALL_DRAW_WINDOW 0x7
#define SYSCALL_GET_KEY 0x8

typedef struct
{
    unsigned int x;
    unsigned int y;
    unsigned short color;
    char c;
    unsigned short scale;
} sys_put_c;

typedef enum
{
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
    void *data;
    uint64_t data_len;
} sys_file_action;

typedef struct
{
    uint64_t id;
    uint32_t width;
    uint32_t height;
    uint32_t *buffer;
} sys_draw_window;

typedef struct
{
    uint32_t last_key;
    uint32_t was_pressed;
} sys_get_key;

#endif