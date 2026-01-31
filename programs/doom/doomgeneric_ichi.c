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
    int doom_w = 640;
    int doom_h = 400;

    syscall_draw_window(doom_w, doom_h, (uint32_t *)DG_ScreenBuffer);
}

void DG_SleepMs(uint32_t ms)
{
    syscall_sleep(ms);
}

uint32_t DG_GetTicksMs()
{
    return (uint32_t)syscall_get_uptime();
}

unsigned char *_last_key;
unsigned char *_was_pressed;

#include "doomkeys.h"

unsigned char convert_to_doom_key(uint32_t ascii_key)
{
    if (ascii_key >= 'A' && ascii_key <= 'Z')
    {
        ascii_key += ('a' - 'A');
    }

    switch (ascii_key)
    {
    case 'w':
        return KEY_UPARROW;
    case 's':
        return KEY_DOWNARROW;
    case 'a':
        return KEY_LEFTARROW;
    case 'd':
        return KEY_RIGHTARROW;
    case ' ':
        return KEY_USE;
    case 'r':
        return KEY_FIRE;
    case 13:
        return KEY_ENTER;
    case 27:
        return KEY_ESCAPE;
    default:
        return (unsigned char)0;
    }
}

int DG_GetKey(int *pressed, unsigned char *key)
{
    uint32_t k, p;
    syscall_get_key(&k, &p);
    *key = convert_to_doom_key(k);
    *pressed = (int)p;

    if (*key != 0)
    {
        return 1;
    }

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

    puts("Created doomgeneric");

    while (1)
    {
        doomgeneric_Tick();
    }

    while (1)
    {
        puts("Doom finished...");
    }
}