/// Very incomplete implementation of syscalls, rushed for minimal doom

#include "syscall.h"
#include "vmm.h"
#include "vfs.h"
#include "../intrp/pit.h"
#include "assembly.h"
#include "serial.h"
#include "../../../sys_common/sys_structs.h"
#include "scheduler.h"
#include "keyboard.h"

typedef void (*syscall_handler_t)(void *ptr);

framebuffer *_fb;
syscall_handler_t _syscalls[1000] = {0};

inline void syscall(uint64_t id, void *ptr)
{
    interrupt80(id, ptr);
}

void syscall_handler(uint64_t syscall_no, void *ptr)
{
    if (syscall_no < 1000 && _syscalls[syscall_no] && (!IS_HIGHER_HALF(ptr)))
    {
        _syscalls[syscall_no](ptr);
    }
}

void sh_hello()
{
    qemu_log("Hello from syscall_handler");
}

void sh_sleep_exit(void *ptr)
{
    process_ctx *p = (process_ctx *)ptr;
    process_unblock(p);
}

void sh_sleep(void *ptr)
{
    uint64_t time = (uint64_t)ptr;

    process_ctx *cur = scheduler_get_active();
    pit_subscribe(sh_sleep_exit, cur, (uint32_t)time);
    process_block(cur);
}

void sh_echo(void *ptr)
{
    qemu_log(ptr);
}

void sh_draw_char(void *ptr)
{
    sys_put_c *c = (sys_put_c *)ptr;
    framebuffer_draw_char8x8(_fb, c->x, c->y, c->c, c->color, c->scale);
}

void sh_puts(void *ptr)
{
    qemu_puts((char *)ptr);
}

void sh_get_uptime(void *ptr)
{
    uint64_t *uptime = (uint64_t *)ptr;
    *uptime = pit_get_current_time_ms();
}

void sh_file_ops(void *ptr)
{
    sys_file_action *action = (sys_file_action *)ptr;

    switch (action->action)
    {
    case SYS_FILE_OPEN:
    {
        file *f = fopen(action->data, READ);
        action->id = (uint64_t)f;
        break;
    }
    case SYS_FILE_CLOSE:
    {
        file *f = (file *)action->id;
        fclose(f);
        break;
    }
    case SYS_FILE_READ:
    {
        file *f = (file *)action->id;
        action->data_len = fread(action->data, 1, action->data_len, f);
        break;
    }
    case SYS_FILE_SEEK:
    {
        file *f = (file *)action->id;
        uint64_t res = fseek(f, action->data_len, (seek_mode)action->data);
        action->data_len = res;
        break;
    }
    case SYS_FILE_TELL:
    {
        file *f = (file *)action->id;
        action->data_len = ftell(f);
        break;
    }
    default:
        break;
    }
}

void sh_draw_window(void *ptr)
{
    sys_draw_window *data = (sys_draw_window *)ptr;

    framebuffer_draw_window(_fb, data->width, data->height, data->buffer);
}

typedef struct
{
    uint32_t key;
    uint32_t pressed;
} key_event;

#define KEY_EVENTS_QUEUE_SIZE 256

key_event *key_events = NULL;
uint32_t key_events_write_index = 0;
uint32_t key_events_read_index = 0;

BOOL kybrd_is_recorded(uint32_t key, BOOL pressed)
{
    BOOL already_in_queue = FALSE;
    uint32_t i = key_events_read_index;
    while (i != key_events_write_index)
    {
        if (key_events[i].key == key && key_events[i].pressed == pressed)
        {
            already_in_queue = TRUE;
            break;
        }
        i = (i + 1) % KEY_EVENTS_QUEUE_SIZE;
    }

    return already_in_queue;
}

void kybrd_press_callback(uint8_t scancode, BOOL pressed)
{
    uint32_t key = kybrd_key_to_ascii(scancode);

    if (!kybrd_is_recorded(key, pressed))
    {
        key_events[key_events_write_index].key = key;
        key_events[key_events_write_index].pressed = pressed;
        ++key_events_write_index;
        key_events_write_index %= KEY_EVENTS_QUEUE_SIZE;
    }
}

void sh_get_key(void *ptr)
{
    sys_get_key *data = (sys_get_key *)ptr;

    if (key_events_read_index == key_events_write_index)
    {
        data->last_key = 0;
        data->was_pressed = 0;
        return;
    }

    data->last_key = key_events[key_events_read_index].key;
    data->was_pressed = key_events[key_events_read_index].pressed;

    ++key_events_read_index;
    key_events_read_index %= KEY_EVENTS_QUEUE_SIZE;
}

void syscall_init(framebuffer *fb)
{
    _fb = fb;
    key_events = (key_event *)kmalloc(sizeof(key_event) * KEY_EVENTS_QUEUE_SIZE);
    kybrd_set_event_callback(kybrd_press_callback);

    _syscalls[0] = (syscall_handler_t)sh_hello;
    _syscalls[1] = (syscall_handler_t)sh_sleep;
    _syscalls[2] = (syscall_handler_t)sh_echo;
    _syscalls[3] = (syscall_handler_t)sh_draw_char;
    _syscalls[4] = (syscall_handler_t)sh_puts;
    _syscalls[5] = (syscall_handler_t)sh_get_uptime;
    _syscalls[6] = (syscall_handler_t)sh_file_ops;
    _syscalls[7] = (syscall_handler_t)sh_draw_window;
    _syscalls[8] = (syscall_handler_t)sh_get_key;
}
