#include "vga.h"

/*
0xA0000 - 0xBFFFF Video Memory used for graphics modes
0xB0000 - 0xB7777 Monochrome Text mode
0xB8000 - 0xBFFFF Color text mode and CGA compatible graphics modes
*/

void vga_put(vga_text_input* input) {
  volatile char *vga_buf = (char *) 0xffffffff800b8000;

  vga_buf += (input->y * VGA_COLUMNS_NUM + input->x) * 2;

  const char* text_index = input->text;

	while(*text_index) {
		*vga_buf = *text_index;
		*(vga_buf + 1) = input->color; 
        
    ++text_index;
    vga_buf += 2;
	}
}

void vga_clear_screen() {
  volatile char *vga_buf = (char *)0xb8000;

  for (int i = 0; i < VGA_COLUMNS_NUM * VGA_ROWS_NUM * 2; i++) {
    vga_buf[i] = '\0';
  }
}
