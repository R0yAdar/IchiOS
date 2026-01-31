#include "sysapi.h"
#include "stdio.h"

unsigned short color = 0xF800;

void _start() {
    const char* str = "Hello from _start!";
    syscall(2, str);
    main();
}

void draw_string(char* str) {
    sys_put_c c = {0, 0, color, *str, 2};
    
    while (*str != 0)
    {
        c.c = *str;
        syscall(3, &c);
        c.x += c.scale * 8;
        c.y += c.scale * 8;
        c.color = color;
        color += 0x15;
        ++str;
    }
}

void main() {
    const char* str = "Hello from userland!";
    while(1) { 
        syscall(0, NULL);
		syscall(1, 500);
        syscall(2, str);

        draw_string(str);

        printf("Uptime is %dms", syscall_get_uptime());
        uint32_t key = 0;
        uint32_t pressed = 0;
        syscall_get_key(&key, &pressed);
        printf("Key is %d, pressed: %d", key, pressed);
	}
}
