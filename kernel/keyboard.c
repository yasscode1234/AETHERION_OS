#include "keyboard.h"

// Simulation : remplacer par un driver réel PS/2 plus tard
void keyboard_init() { }

// Retourne le caractère tapé, 0 si aucune touche
char keyboard_get_char() {
    return 0; // Simulation : aucune touche
}

// Retourne la flèche appuyée : 1 = ↑, -1 = ↓, 0 = rien
int keyboard_arrow() {
    int dir = 0; // Simulation : aucune flèche

    // Sécurisation : ne jamais dépasser les bornes
    if(dir > 1) dir = 1;
    else if(dir < -1) dir = -1;

    return dir;
}
