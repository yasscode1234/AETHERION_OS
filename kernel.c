/*
 * AETHERION OS - Kernel
 * Copyright (C) 2025 yasscode
 * Licensed under GPLv2
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// --- VGA ---
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
        if (cursor_x >= 50) { // limite zone texte principale
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

// --- Zone IA droite ---
void print_ia(const char* str) {
    int start_x = 50;
    int start_y = 2;
    for (int i=0; str[i]!='\0'; i++) {
        int index = start_y*VGA_WIDTH + start_x + i;
        VGA_BUFFER[index] = ((uint16_t)0x0A << 8) | str[i]; // vert clair
    }
}

// --- Paramètres système ---
typedef struct {
    uint8_t brightness;
    uint8_t volume;
    uint8_t language; // 0=FR,1=EN
} system_params_t;

system_params_t sys_params = {10, 5, 0};

// --- Réponses IA ---
const char* reponses_ia[] = {
    "Bonjour, je suis votre assistant AETHERION OS.",
    "Tape 'help' pour la liste des commandes.",
    "Je peux gerer les parametres et les mises a jour."
};

void repondre_ia(int index) {
    if(index < 0 || index >= sizeof(reponses_ia)/sizeof(reponses_ia[0])) return;
    print_ia(reponses_ia[index]);
}

// --- Commandes IA ---
void ia_command(const char* cmd) {
    char buf[80];

    if (strcmp(cmd, "help") == 0) print_ia("Commandes: help, update, settings, set, show");
    else if (strcmp(cmd, "update") == 0) {
        print_ia("Recherche mise a jour...");
        // Simuler attente
        for(volatile int i=0;i<1000000;i++);
        print_ia("Mise a jour disponible: AETHERION OS v0.1");
    }
    else if (strcmp(cmd, "settings") == 0) {
        snprintf(buf, 80, "Brightness=%d Volume=%d Language=%d", 
                 sys_params.brightness, sys_params.volume, sys_params.language);
        print_ia(buf);
    }
    else if (strncmp(cmd, "set ", 4) == 0) {
        char param[32]; int value;
        if (sscanf(cmd + 4, "%31s %d", param, &value) == 2) {
            if (strcmp(param,"brightness")==0) sys_params.brightness=value;
            else if (strcmp(param,"volume")==0) sys_params.volume=value;
            else if (strcmp(param,"language")==0) sys_params.language=value;
            else { print_ia("Parametre inconnu."); return; }
            print_ia("Parametre modifie.");
        } else print_ia("Erreur: set <param> <valeur>");
    }
    else if (strcmp(cmd,"show")==0) {
        snprintf(buf, 80, "Brightness=%d Volume=%d Language=%d", 
                 sys_params.brightness, sys_params.volume, sys_params.language);
        print_ia(buf);
    }
    else print_ia("Commande inconnue.");
}

// --- Saisie clavier ---
char buffer[64];
int buffer_index = 0;

char read_key() {
    // Simulation clavier, à remplacer par code PS/2 réel
    return 0;
}

void ia_input_loop() {
    char c;
    while ((c = read_key()) != 0) {
        if (c == '\n') {
            buffer[buffer_index] = '\0';
            ia_command(buffer);
            buffer_index = 0;
        } else {
            buffer[buffer_index++] = c;
            if (buffer_index >= 64) buffer_index = 63;
        }
    }
}

// --- Kernel principal ---
void kernel_main() {
    for(int i=0;i<VGA_WIDTH*VGA_HEIGHT;i++)
        VGA_BUFFER[i] = (0x0F << 8) | ' ';

    print("Bienvenue sur AETHERION OS !\n", 0x0F);
    print("Initialisation du module IA...\n", 0x0A);
    repondre_ia(0);

    while(1) {
        ia_input_loop(); // traiter saisie clavier
    }
}
