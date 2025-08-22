#include "ia.h"
#include "vga.h"
#include "system.h"
#include <string.h>

#define HIST_SIZE 10
char history[HIST_SIZE][64];
int hist_count=0, hist_index=0;

const char* reponses_random[] = {
    "Je ne comprends pas...",
    "Essaie 'help' pour voir les commandes.",
    "Tout fonctionne normalement.",
    "AETHERION OS est prêt.",
    "Parametres a jour."
};

void add_history(const char* cmd){
    int i;
    if(hist_count < HIST_SIZE){
        strcpy(history[hist_count++], cmd);
    } else {
        for(i=1;i<HIST_SIZE;i++) strcpy(history[i-1], history[i]);
        strcpy(history[HIST_SIZE-1], cmd);
    }
    hist_index = hist_count;
}

int rand_simple(){ static unsigned int seed=12345; seed=seed*1103515245+12345; return (seed/65536)%5; }

void ia_print_random(){ vga_print(reponses_random[rand_simple()], 0x0A); }

void ia_execute_command(const char* cmd){
    char tmp[64];
    if(strcmp(cmd,"help")==0) vga_print("Commandes: help, update, settings, set, show\n", 0x0A);
    else if(strcmp(cmd,"update")==0){
        vga_print("Recherche mise a jour...\n", 0x0A);
        for(volatile int i=0;i<1000000;i++);
        vga_print("Mise a jour disponible: AETHERION OS v0.1\n", 0x0A);
    }
    else if(strcmp(cmd,"settings")==0){
        sprintf(tmp, "brightness=%d volume=%d language=%d\n", sys_params.brightness, sys_params.volume, sys_params.language);
        vga_print(tmp,0x0A);
    }
    else if(strncmp(cmd,"set ",4)==0){
        char p[16]; int v; sscanf(cmd,"set %s %d", p,&v);
        if(strcmp(p,"brightness")==0) sys_params.brightness=v;
        else if(strcmp(p,"volume")==0) sys_params.volume=v;
        else if(strcmp(p,"language")==0) sys_params.language=v;
        vga_print("Parametre modifie\n",0x0A);
    }
    else ia_print_random();

    add_history(cmd);
}

void ia_history_nav(int dir, char* buffer, int* index){
    if(hist_count==0) return;
    if(dir==1){ // flèche ↑
        if(*index>0) (*index)--;
    } else if(dir==-1){ // flèche ↓
        if(*index<hist_count-1) (*index)++;
    }
    strcpy(buffer, history[*index]);
}
