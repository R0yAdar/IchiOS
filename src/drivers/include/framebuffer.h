#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "types.h"

typedef struct {
    uint8_t* fb;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t bpp;
} framebuffer;

framebuffer* framebuffer_init(uint8_t* fb, uint32_t width, uint32_t height, uint32_t pitch, uint8_t bpp, uint32_t model);

void framebuffer_put_pixel(framebuffer* fb, uint32_t x, uint32_t y, uint16_t color);

void framebuffer_clear(framebuffer* fb);

#endif