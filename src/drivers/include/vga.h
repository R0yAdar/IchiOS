#pragma once

#ifndef VGA_H
#define VGA_H

#define VGA_COLUMNS_NUM 80
#define VGA_ROWS_NUM 25

typedef struct
{
  int x;
  int y;
  const char* text;
  char color;
} vga_text_input;

void vga_put(vga_text_input* input);

void vga_clear_screen();

#endif