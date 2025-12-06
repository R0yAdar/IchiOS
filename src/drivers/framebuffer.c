#include "framebuffer.h"
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

void framebuffer_clear(framebuffer* fb) {
    for (size_t i = 0; i < fb->pitch * fb->height; i++) {
        ((uint32_t*)fb->fb)[i] = 0;
    }
}