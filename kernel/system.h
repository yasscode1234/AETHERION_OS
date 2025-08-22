/*
 * Paramètres système
 * Copyright (C) 2025 yasscode
 * Licence GPLv2
 */

#ifndef SYSTEM_H
#define SYSTEM_H

typedef struct {
    unsigned char brightness;
    unsigned char volume;
    unsigned char language;
} system_params_t;

extern system_params_t sys_params;

#endif
