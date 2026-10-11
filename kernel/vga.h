#ifndef AETHERION_VGA_H
#define AETHERION_VGA_H

#include "kernel.h"

/* ---- Couleurs VGA ---- */
typedef enum {
    VGA_BLACK = 0,
    VGA_BLUE,
    VGA_GREEN,
    VGA_CYAN,
    VGA_RED,
    VGA_MAGENTA,
    VGA_BROWN,
    VGA_LIGHT_GREY,
    VGA_DARK_GREY,
    VGA_LIGHT_BLUE,
    VGA_LIGHT_GREEN,
    VGA_LIGHT_CYAN,
    VGA_LIGHT_RED,
    VGA_LIGHT_MAGENTA,
    VGA_YELLOW,
    VGA_WHITE
} vga_color_t;

/* ---- API VGA / Terminal ---- */
void vga_init(void);
void vga_clear(void);
void vga_putchar(char c);
void vga_write(const char* str);
void vga_set_color(vga_color_t fg, vga_color_t bg);
void vga_set_cursor(size_t row, size_t col);
void vga_get_cursor(size_t* row, size_t* col);
void vga_printf(const char* fmt, ...);
void vga_scroll_up(int lines);

#endif /* AETHERION_VGA_H */
