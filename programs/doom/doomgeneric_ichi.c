#include "doomgeneric.h"
#include <stdio.h>
#include <stdint.h>

#include "sysapi.h"

int net_client_connected = 0;
int drone = 0;

void DG_Init()
{
    puts("Initializing...");
}

void DG_DrawFrame()
{
    // DG_ScreenBuffer is a 320x200 uint32_t array
    // Copy it to your OS screen memory
    puts("Drawing frame...");
}

void DG_SleepMs(uint32_t ms)
{
    syscall_sleep(ms);
}

uint32_t DG_GetTicksMs()
{
    return (uint32_t)syscall_get_uptime();
}

int DG_GetKey(int *pressed, unsigned char *key)
{
    return 0;
}

void DG_SetWindowTitle(const char *title) {}

void _start()
{
    puts("Hello from _start!");

    char *argv[] = {
        "doom.elf",
        "-iwad",
        "/files/doom1.wad",
        NULL};

    int argc = 3;

    doomgeneric_Create(argc, argv);

    puts("CReAtED");

    while (1)
    {
        doomgeneric_Tick();
    }

    while (1)
    {
        puts("Doom finished...");
    }
}