#ifndef VGA_H
#define VGA_H

#include "types.h"

void vga_init();
void vga_clear_screen();
void vga_draw_rect(int x,int y,int w,int h,uint8_t color);
void vga_draw_gradient(int x,int y,int w,int h,uint8_t start,uint8_t end);
void vga_draw_border(int x,int y,int w,int h,uint8_t color);
void vga_fade_in(int x,int y,int w,int h,uint8_t start,uint8_t end);
void vga_draw_text(int x,int y,const char* str,uint8_t color);
void vga_draw_cursor(int x,int y,uint8_t color);
void vga_flash_border(int x,int y,int w,int h,uint8_t color);

#endif
