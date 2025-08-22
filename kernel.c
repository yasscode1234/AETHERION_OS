/*
 * AETHERION OS - Kernel
 * Copyright (C) 2025 yasscode
 * Licensed under GPLv2
 */

#include <stdint.h>

volatile uint16_t* VGA_BUFFER = (uint16_t*)0xB8000;
const int VGA_WIDTH = 80;
const int VGA_HEIGHT = 25;
uint8_t cursor_x = 0;
uint8_t cursor_y = 0;

void put_char(char c, uint8_t color) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else {
        const int index = cursor_y * VGA_WIDTH + cursor_x;
        VGA_BUFFER[index] = ((uint16_t)color << 8) | c;
        cursor_x++;
        if (cursor_x >= 50) {
            cursor_x = 0;
            cursor_y++;
        }
    }
}

void print(const char* str, uint8_t color) {
    for (int i = 0; str[i] != '\0'; i++) {
        put_char(str[i], color);
    }
}

void print_ia(const char* str) {
    int start_x = 50;
    int start_y = 2;
    for(int i=0; str[i]!='\0'; i++) {
        int index = start_y*VGA_WIDTH + start_x + i;
        VGA_BUFFER[index] = ((uint16_t)0x0A << 8) | str[i];
    }
}

const char* reponses_ia[] = {
    "Bonjour, je suis votre assistant AETHERION OS.",
    "Je
