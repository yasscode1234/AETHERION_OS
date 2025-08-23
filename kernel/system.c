#include "system.h"

// Paramètres système par défaut
system_params_t sys_params = {10, 5, 0};

// Fonctions pour sécuriser la modification des paramètres
void set_brightness(unsigned char value) {
    if(value > 100) value = 100;
    sys_params.brightness = value;
}

void set_volume(unsigned char value) {
    if(value > 100) value = 100;
    sys_params.volume = value;
}

void set_language(unsigned char value) {
    // Limite par exemple à 0-3 langues disponibles
    if(value > 3) value = 3;
    sys_params.language = value;
}

unsigned char get_brightness() { return sys_params.brightness; }
unsigned char get_volume()     { return sys_params.volume; }
unsigned char get_language()   { return sys_params.language; }
