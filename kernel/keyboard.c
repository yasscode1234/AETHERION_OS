#include "keyboard.h"

// Simulation : à remplacer par driver réel PS/2
void keyboard_init() { }

char keyboard_get_char() {
    return 0; // Retourne 0 si aucune touche, sinon le char tapé
}

int keyboard_arrow() {
    return 0; // Retourne 1 pour ↑, -1 pour ↓, 0 sinon
}
