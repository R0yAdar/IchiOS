#include "doomgeneric.h"
#include <stdio.h>
#include <stdint.h>

int net_client_connected = 0;
int drone = 0;

// 4. Mandatory doomgeneric Implementations
void DG_Init() {
    // Init your framebuffer here
}

void DG_DrawFrame() {
    // DG_ScreenBuffer is a 320x200 uint32_t array
    // Copy it to your OS screen memory
}

void DG_SleepMs(uint32_t ms) {
    // Your OS sleep function
}

uint32_t DG_GetTicksMs() {
    // Your OS uptime in milliseconds
    return 0; 
}

int DG_GetKey(int* pressed, unsigned char* key) {
    return 0; // Return 1 if a key event happened
}

void DG_SetWindowTitle(const char * title) {}

void _start() {
    puts("Hello from _start!");
    while (1)
    {
        /* code */
    }
} 