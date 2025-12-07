#include "framebuffer.h"
#include "font.h"
#include "vmm.h"

framebuffer* framebuffer_init(uint8_t* fb, uint32_t width, uint32_t height, uint32_t pitch, uint8_t bpp, uint32_t model) {
    if (!fb) return NULL;
    if (bpp != 15 && model != 6) return NULL;

    framebuffer* fb_out = (framebuffer*)kmalloc(sizeof(framebuffer));
    if (!fb_out) return NULL;

    fb_out->fb = fb;
    fb_out->width = width;
    fb_out->height = height;
    fb_out->pitch = pitch;
    fb_out->bpp = bpp;
    return fb_out;
}

void framebuffer_put_pixel(framebuffer* fb, uint32_t x, uint32_t y, uint16_t color) {
    if (x >= fb->width || y >= fb->height) return;

    uint32_t offset = fb->pitch * y + x * ((fb->bpp + 7) / 8);

    *(uint16_t*)((uint8_t*)fb->fb + offset) = color & 0xFFFE;
}

void framebuffer_draw_char8x8(framebuffer* fb, uint32_t x, uint32_t y, char c, uint16_t color, uint8_t scale) {
    if (x + FONT8X8_BASIC_WIDTH >= fb->width || y + FONT8X8_BASIC_HEIGHT >= fb->height) return;

    for (size_t row = 0; row < FONT8X8_BASIC_HEIGHT; row++) {
        for (size_t col = 0; col < FONT8X8_BASIC_WIDTH; col++) {
            if (font8x8_basic[(uint8_t)c][row] & (1 << col)) {
                for (size_t xscale = 0; xscale < scale; xscale++)
                {
                    for (size_t yscale = 0; yscale < scale; yscale++)
                    {
                        framebuffer_put_pixel(fb, x + col * scale + xscale, y + row * scale + yscale, color);
                    }
                }
            }
        }
    }
}

void framebuffer_clear(framebuffer* fb) {
    for (size_t i = 0; i < fb->pitch * fb->height; i++) {
        ((uint8_t*)fb->fb)[i] = 0;
    }
}