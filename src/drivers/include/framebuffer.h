#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "types.h"

typedef struct {
    uint8_t* image;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
} framebuffer;

framebuffer* framebuffer_init(uint8_t* base, uint32_t width, uint32_t height, uint32_t pitch);

void framebuffer_put(framebuffer* fb, uint32_t x, uint32_t y, uint32_t color);

void framebuffer_clear(framebuffer* fb, uint32_t color);

#endif