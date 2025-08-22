/*
 * Driver clavier PS/2
 * Copyright (C) 2025 yasscode
 * Licence GPLv2
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

void keyboard_init();
char keyboard_get_char();
int keyboard_arrow(); // 1 = ↑, -1 = ↓, 0 = rien

#endif
