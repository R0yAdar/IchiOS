#include "syscall.h"
#include "vmm.h"
#include "../intrp/pit.h"
#include "assembly.h"
#include "serial.h"
#include "../../../include/sys_structs.h"

typedef void(*syscall_handler_t)(void* ptr);

framebuffer* _fb;
syscall_handler_t _syscalls[1000] = {0};

uint64_t get_time() {
    return pit_get_current_time_ms();
}

inline void syscall(uint64_t id, void* ptr)
{
    interrupt80(id, ptr);
}

void syscall_handler(uint64_t syscall_no, void* ptr) {
    if (syscall_no < 1000 && _syscalls[syscall_no] && (!IS_HIGHER_HALF(ptr))) {
        _syscalls[syscall_no](ptr);
    }
}

void draw_screen() {

    
}

void sh_hello() {
    qemu_log("Hello from syscall_handler");
}

void sh_sleep() {
    sleep(1000);
}

void sh_echo(void* ptr) {
    qemu_log(ptr);
}



void sh_draw_char(void* ptr) {
    sys_put_c* c = (sys_put_c*)ptr;
    framebuffer_draw_char8x8(_fb, c->x, c->y, c->c, c->color, c->scale);
}

void syscall_init(framebuffer* fb) {
    _fb = fb;
    _syscalls[0] = (syscall_handler_t)sh_hello;
    _syscalls[1] = (syscall_handler_t)sh_sleep;
    _syscalls[2] = (syscall_handler_t)sh_echo;
    _syscalls[3] = (syscall_handler_t)sh_draw_char;
}
