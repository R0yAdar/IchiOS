#include "../include/sysapi.h"

void main() {
    while(1) { 
    		syscall(0, NULL);
			syscall(1, NULL);
	}
}