#include "../include/sysapi.h"

void _start() {
    const char* str = "Hello from _start!";
    syscall(2, str);
    main();
}

void main() {
    const char* str = "Hello from userland!";
    while(1) { 
        syscall(0, NULL);
		syscall(1, NULL);
        syscall(2, str);
	}
}
