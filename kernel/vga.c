#include "vga.h"
#include <stdint.h>

volatile uint16_t* VGA_BUFFER = (uint16_t*)0xB8000;
int cursor_x = 0, cursor_y = 0;
const int VGA_WIDTH = 80;

void vga_put_char(char c, unsigned char color) {
    if(c == '\n'){ cursor_x=0; cursor_y++; return; }
    int idx = cursor_y*VGA_WIDTH + cursor_x;
    VGA_BUFFER[idx] = ((uint16_t)color << 8) | c;
    cursor_x++;
    if(cursor_x >= 50){ cursor_x=0; cursor_y++; }
}

void vga_print(const char* str, unsigned char color){
    while(*str) vga_put_char(*str++, color);
}

void vga_clear_screen(){
    for(int i=0;i<VGA_WIDTH*25;i++) VGA_BUFFER[i] = ((uint16_t)0x0F << 8) | ' ';
    cursor_x=0; cursor_y=0;
}
