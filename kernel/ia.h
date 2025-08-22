/*
 * IA interactive
 * Copyright (C) 2025 yasscode
 * Licence GPLv2
 */

#ifndef IA_H
#define IA_H

void ia_print_random();
void ia_execute_command(const char* cmd);
void ia_history_nav(int dir, char* buffer, int* index);

#endif
