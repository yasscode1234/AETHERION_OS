#include "vga.h"

volatile uint16_t* VGA_BUFFER = (uint16_t*)0xB8000;
int VGA_WIDTH = 80;
int VGA_HEIGHT = 25;

static inline int clamp(int val, int min, int max) {
    if(val < min) return min;
    if(val > max) return max;
    return val;
}

void vga_init() {
    vga_clear_screen();
}

void vga_clear_screen() {
    for(int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        VGA_BUFFER[i] = (0x0F << 8) | ' ';
}

void vga_draw_rect(int x, int y, int w, int h, uint8_t color) {
    x = clamp(x, 0, VGA_WIDTH-1);
    y = clamp(y, 0, VGA_HEIGHT-1);
    w = clamp(w, 0, VGA_WIDTH-x);
    h = clamp(h, 0, VGA_HEIGHT-y);

    for(int j = 0; j < h; j++) {
        for(int i = 0; i < w; i++) {
            int idx = (y+j)*VGA_WIDTH + (x+i);
            VGA_BUFFER[idx] = (color << 8) | ' ';
        }
    }
}

void vga_draw_gradient(int x, int y, int w, int h, uint8_t start, uint8_t end) {
    x = clamp(x, 0, VGA_WIDTH-1);
    y = clamp(y, 0, VGA_HEIGHT-1);
    w = clamp(w, 0, VGA_WIDTH-x);
    h = clamp(h, 0, VGA_HEIGHT-y);

    for(int j = 0; j < h; j++) {
        uint8_t color = start + (end - start) * j / h;
        for(int i = 0; i < w; i++) {
            int idx = (y+j)*VGA_WIDTH + (x+i);
            VGA_BUFFER[idx] = (color << 8) | ' ';
        }
    }
}

void vga_draw_border(int x, int y, int w, int h, uint8_t color) {
    x = clamp(x, 0, VGA_WIDTH-1);
    y = clamp(y, 0, VGA_HEIGHT-1);
    w = clamp(w, 0, VGA_WIDTH-x);
    h = clamp(h, 0, VGA_HEIGHT-y);

    for(int i = 0; i < w; i++) {
        VGA_BUFFER[y*VGA_WIDTH + x + i] = (color << 8) | ' ';
        VGA_BUFFER[(y+h-1)*VGA_WIDTH + x + i] = (color << 8) | ' ';
    }
    for(int i = 0; i < h; i++) {
        VGA_BUFFER[(y+i)*VGA_WIDTH + x] = (color << 8) | ' ';
        VGA_BUFFER[(y+i)*VGA_WIDTH + x + w - 1] = (color << 8) | ' ';
    }
}

void vga_fade_in(int x, int y, int w, int h, uint8_t start, uint8_t end) {
    for(uint8_t c = start; c <= end; c++) {
        vga_draw_rect(x, y, w, h, c);
        for(volatile int delay = 0; delay < 100000; delay++);
    }
}

void vga_draw_text(int x, int y, const char* str, uint8_t color) {
    x = clamp(x, 0, VGA_WIDTH-1);
    y = clamp(y, 0, VGA_HEIGHT-1);

    while(*str) {
        if(x >= VGA_WIDTH) { x = 0; y++; if(y >= VGA_HEIGHT) break; }
        VGA_BUFFER[y*VGA_WIDTH + x] = (color << 8) | *str++;
        x++;
    }
}

void vga_draw_cursor(int x, int y, uint8_t color) {
    x = clamp(x, 0, VGA_WIDTH-1);
    y = clamp(y, 0, VGA_HEIGHT-1);
    VGA_BUFFER[y*VGA_WIDTH + x] = (color << 8) | '_';
}

void vga_flash_border(int x, int y, int w, int h, uint8_t color) {
    for(int i = 0; i < 5; i++) {
        vga_draw_border(x, y, w, h, color);
        for(volatile int j = 0; j < 50000; j++);
        vga_draw_border(x, y, w, h, 0x0);
        for(volatile int j = 0; j < 50000; j++);
    }
}
