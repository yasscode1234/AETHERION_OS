#include "ia.h"
#include "vga.h"
#include "system.h"
#include <string.h>
#include <stdio.h>

#define HIST_SIZE 10
#define CMD_BUFFER_SIZE 64

static char history[HIST_SIZE][CMD_BUFFER_SIZE];
static int hist_count = 0;
static int hist_index = 0;

static const char* reponses_random[] = {
    "Je ne comprends pas...",
    "Essaie 'help' pour voir les commandes.",
    "Tout fonctionne normalement.",
    "AETHERION OS est prêt.",
    "Parametres a jour."
};

// Ajoute une commande à l'historique
static void add_history(const char* cmd) {
    int i;
    if(hist_count < HIST_SIZE) {
        strncpy(history[hist_count++], cmd, CMD_BUFFER_SIZE-1);
        history[hist_count-1][CMD_BUFFER_SIZE-1] = '\0';
    } else {
        for(i=1; i<HIST_SIZE; i++)
            strncpy(history[i-1], history[i], CMD_BUFFER_SIZE-1);
        strncpy(history[HIST_SIZE-1], cmd, CMD_BUFFER_SIZE-1);
        history[HIST_SIZE-1][CMD_BUFFER_SIZE-1] = '\0';
    }
    hist_index = hist_count;
}

// Générateur pseudo-aléatoire simple
static int rand_simple() {
    static unsigned int seed = 12345;
    seed = seed * 1103515245 + 12345;
    return (seed / 65536) % 5;
}

// Affiche une réponse aléatoire
void ia_print_random() {
    vga_draw_text(51, 2, reponses_random[rand_simple()], 0x0A);
}

// Exécute une commande entrée par l'utilisateur
void ia_execute_command(const char* cmd) {
    char tmp[CMD_BUFFER_SIZE];

    if(strcmp(cmd, "help") == 0) {
        vga_draw_text(51, 3, "Commandes: help, update, settings, set, show", 0x0A);
    } else if(strcmp(cmd, "update") == 0) {
        vga_draw_text(51, 3, "Recherche mise a jour...", 0x0A);
        for(volatile int i=0; i<1000000; i++);
        vga_draw_text(51, 4, "Mise a jour disponible: AETHERION OS v0.1", 0x0A);
    } else if(strcmp(cmd, "settings") == 0) {
        snprintf(tmp, CMD_BUFFER_SIZE, "brightness=%d volume=%d language=%d",
                 sys_params.brightness, sys_params.volume, sys_params.language);
        vga_draw_text(51, 3, tmp, 0x0A);
    } else if(strncmp(cmd, "set ", 4) == 0) {
        char p[16]; int v;
        if(sscanf(cmd, "set %15s %d", p, &v) == 2) {
            if(strcmp(p, "brightness") == 0) sys_params.brightness = v;
            else if(strcmp(p, "volume") == 0) sys_params.volume = v;
            else if(strcmp(p, "language") == 0) sys_params.language = v;
            vga_draw_text(51, 3, "Parametre modifie", 0x0A);
        } else {
            vga_draw_text(51, 3, "Syntaxe invalide", 0x0A);
        }
    } else {
        ia_print_random();
    }

    add_history(cmd);
}

// Navigation dans l'historique
void ia_history_nav(int dir, char* buffer, int* index) {
    if(hist_count == 0) return;
    if(dir == 1 && *index > 0) {
        (*index)--;
    } else if(dir == -1 && *index < hist_count-1) {
        (*index)++;
    }
    strncpy(buffer, history[*index], CMD_BUFFER_SIZE-1);
    buffer[CMD_BUFFER_SIZE-1] = '\0';
}
