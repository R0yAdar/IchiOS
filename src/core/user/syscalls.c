#include "../intrp/pit.h"
#include "assembly.h"
#include "serial.h"

uint64_t get_time() {
    return pit_get_current_time_ms();
}

inline void syscall(uint64_t id, void* ptr)
{
    interrupt80(id, ptr);
}

void syscall_handler(uint64_t syscall_no, void* ptr) {    
    if (syscall_no == 0) {
        qemu_log("Hello from syscall_handler");
    } else if (syscall_no == 1) {
        sleep(1000);
    } else if (syscall_no == 2) {
        qemu_log(ptr);
    }
}
