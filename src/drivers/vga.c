#include "vga.h"
#include "vmm.h"

#define VGA_COLOR_TEXT_MODE_ADDRESS ((void *)0xb8000)

void vga_put(vga_text_input *input)
{
  volatile char *vga_buf = (char *)vmm_get_vaddr(VGA_COLOR_TEXT_MODE_ADDRESS);

  vga_buf += (input->y * VGA_COLUMNS_NUM + input->x) * 2;

  const char *text_index = input->text;

  while (*text_index)
  {
    *vga_buf = *text_index;
    *(vga_buf + 1) = input->color;

    ++text_index;
    vga_buf += 2;
  }
}

void vga_clear_screen()
{
  volatile char *vga_buf = (char *)vmm_get_vaddr(VGA_COLOR_TEXT_MODE_ADDRESS);

  for (int i = 0; i < VGA_COLUMNS_NUM * VGA_ROWS_NUM * 2; i++)
  {
    vga_buf[i] = '\0';
  }
}
