#include "vga.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

/* ---- Ecran VGA texte 80x25 ---- */
#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  ((u16*)0xB8000)

static size_t term_row = 0;
static size_t term_col = 0;
static u8     term_color = 0x0F;
static u16*   term_buffer = VGA_MEMORY;

static inline u8 vga_entry_color(vga_color_t fg, vga_color_t bg) {
    return (u8)(fg | (bg << 4));
}

static inline u16 vga_entry(char c, u8 color) {
    return (u16)c | ((u16)color << 8);
}

void vga_init(void) {
    term_buffer = VGA_MEMORY;
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_clear();
}

void vga_set_color(vga_color_t fg, vga_color_t bg) {
    term_color = vga_entry_color(fg, bg);
}

void vga_clear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            term_buffer[y * VGA_WIDTH + x] = vga_entry(' ', term_color);
        }
    }
    term_row = 0;
    term_col = 0;
}

static void vga_scroll(void) {
    for (size_t y = 1; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            term_buffer[(y - 1) * VGA_WIDTH + x] = term_buffer[y * VGA_WIDTH + x];
        }
    }
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        term_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', term_color);
    }
    term_row = VGA_HEIGHT - 1;
}

void vga_scroll_up(int lines) {
    for (int i = 0; i < lines; i++)
        vga_scroll();
}

void vga_putchar(char c) {
    if (c == '\n') {
        term_col = 0;
        if (++term_row == VGA_HEIGHT)
            vga_scroll();
        return;
    }
    if (c == '\r') {
        term_col = 0;
        return;
    }
    if (c == '\t') {
        term_col = (term_col + 4) & ~3;
        if (term_col >= VGA_WIDTH) {
            term_col = 0;
            if (++term_row == VGA_HEIGHT)
                vga_scroll();
        }
        return;
    }
    if (c == '\b') {
        if (term_col > 0) {
            term_col--;
            term_buffer[term_row * VGA_WIDTH + term_col] = vga_entry(' ', term_color);
        }
        return;
    }
    term_buffer[term_row * VGA_WIDTH + term_col] = vga_entry(c, term_color);
    if (++term_col == VGA_WIDTH) {
        term_col = 0;
        if (++term_row == VGA_HEIGHT)
            vga_scroll();
    }
}

void vga_write(const char* str) {
    while (*str)
        vga_putchar(*str++);
}

void vga_set_cursor(size_t row, size_t col) {
    if (row < VGA_HEIGHT && col < VGA_WIDTH) {
        term_row = row;
        term_col = col;
    }
}

void vga_get_cursor(size_t* row, size_t* col) {
    if (row) *row = term_row;
    if (col) *col = term_col;
}

/* ---- printf-like ---- */
void vga_printf(const char* fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    vga_write(buf);
}
