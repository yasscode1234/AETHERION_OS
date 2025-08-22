/*
 * Driver VGA
 * Copyright (C) 2025 yasscode
 * Licence GPLv2
 */

#ifndef VGA_H
#define VGA_H

void vga_clear_screen();
void vga_print(const char* str, unsigned char color);
void vga_put_char(char c, unsigned char color);

#endif
