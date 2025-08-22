/*
 * AETHERION OS - Kernel
 * Copyright (C) 2025 yasscode
 * Licensed under GPLv2
 */

/* --- Types simples ---
 */
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

/* --- VGA ---
 */
volatile uint16_t* VGA_BUFFER = (uint16_t*)0xB8000;
const int VGA_WIDTH = 80;
const int VGA_HEIGHT = 25;
uint8_t cursor_x = 0;
uint8_t cursor_y = 0;

void put_char(char c, uint8_t color){
    if(c=='\n'){ cursor_x=0; cursor_y++; }
    else{
        int index = cursor_y*VGA_WIDTH + cursor_x;
        VGA_BUFFER[index] = ((uint16_t)color<<8)|c;
        cursor_x++;
        if(cursor_x>=50){ cursor_x=0; cursor_y++; }
    }
}

void print(const char* str, uint8_t color){
    while(*str) put_char(*str++, color);
}

/* --- Zone IA droite ---
 */
void print_ia(const char* str){
    int start_x = 50, start_y = 2;
    for(int i=0; str[i]; i++){
        int index = start_y*VGA_WIDTH + start_x + i;
        VGA_BUFFER[index] = ((uint16_t)0x0A<<8)|str[i];
    }
}

/* --- Fonctions chaînes ---
 */
int strcmp(const char* a,const char* b){
    while(*a && (*a==*b)){ a++; b++; }
    return *(unsigned char*)a - *(unsigned char*)b;
}

int strncmp(const char* a,const char* b,int n){
    for(int i=0;i<n;i++){
        if(a[i]!=b[i] || a[i]==0 || b[i]==0) return a[i]-b[i];
    }
    return 0;
}

int strlen(const char* s){ int i=0; while(s[i]) i++; return i; }

void itoa(int value, char* str){
    char buf[10]; int i=0;
    if(value==0){ str[0]='0'; str[1]=0; return; }
    while(value>0){ buf[i++]='0'+(value%10); value/=10; }
    int j=0; while(i>0) str[j++]=buf[--i]; str[j]=0;
}

/* --- Paramètres système ---
 */
typedef struct{
    uint8_t brightness;
    uint8_t volume;
    uint8_t language;
} system_params_t;

system_params_t sys_params = {10,5,0};

/* --- Historique commandes ---
 */
#define HIST_SIZE 10
char history[HIST_SIZE][64];
int hist_count=0, hist_index=0;

void add_to_history(const char* cmd){
    if(hist_count<HIST_SIZE){
        int i=0; while(cmd[i]) history[hist_count][i]=cmd[i++];
        history[hist_count][i]=0; hist_count++;
    } else {
        for(int i=1;i<HIST_SIZE;i++){
            int j=0; while(history[i][j]){ history[i-1][j]=history[i][j]; j++; }
            history[i-1][j]=0;
        }
        int i=0; while(cmd[i]) history[HIST_SIZE-1][i]=cmd[i++]; history[HIST_SIZE-1][i]=0;
    }
    hist_index=hist_count;
}

/* --- Réponses IA ---
 */
const char* reponses_random[] = {
    "Je ne comprends pas...",
    "Essaie 'help' pour voir les commandes.",
    "Tout fonctionne normalement.",
    "AETHERION OS est prêt.",
    "Parametres a jour."
};

int rand_simple(){ static unsigned int seed=12345; seed=seed*1103515245+12345; return (seed/65536)%5; }

void print_ia_random(){ print_ia(reponses_random[rand_simple()]); }

/* --- Commandes IA ---
 */
char buffer[64]; int buffer_index=0;

void ia_command(const char* cmd){
    char tmp[64]; char t[10]; int k;

    if(strcmp(cmd,"help")==0) print_ia("Commandes: help, update, settings, set, show");
    else if(strcmp(cmd,"update")==0){
        print_ia("Recherche mise a jour...");
        for(volatile int i=0;i<1000000;i++);
        print_ia("Mise a jour disponible: AETHERION OS v0.1");
    }
    else if(strcmp(cmd,"settings")==0){
        k=0; itoa(sys_params.brightness,t); int i=0; while(t[i]) tmp[k++]=t[i++];
        tmp[k++]=' '; itoa(sys_params.volume,t); i=0; while(t[i]) tmp[k++]=t[i++];
        tmp[k++]=' '; itoa(sys_params.language,t); i=0; while(t[i]) tmp[k++]=t[i++];
        tmp[k]=0; print_ia(tmp);
    }
    else if(strncmp(cmd,"set ",4)==0){
        char p[16]; int v=0; int i=0,j=0;
        while(cmd[4+i]!=' ' && cmd[4+i]!=0){ p[i]=cmd[4+i]; i++; } p[i]=0;
        i++; while(cmd[4+i]!='\0'){ v=v*10+(cmd[4+i]-'0'); i++; }
        if(strcmp(p,"brightness")==0) sys_params.brightness=v;
        else if(strcmp(p,"volume")==0) sys_params.volume=v;
        else if(strcmp(p,"language")==0) sys_params.language=v;
        print_ia("Parametre modifie.");
    }
    else if(strcmp(cmd,"show")==0){
        k=0; itoa(sys_params.brightness,t); i=0; while(t[i]) tmp[k++]=t[i++];
        tmp[k++]=' '; itoa(sys_params.volume,t); i=0; while(t[i]) tmp[k++]=t[i++];
        tmp[k++]=' '; itoa(sys_params.language,t); i=0; while(t[i]) tmp[k++]=t[i++];
        tmp[k]=0; print_ia(tmp);
    }
    else print_ia_random();
    add_to_history(cmd);
}

/* --- Lecture clavier ---
 */
char read_key(){ return 0; } // Simulation, à remplacer par PS/2 réel

void ia_input_loop(){
    char c;
    while((c=read_key())!=0){
        if(c=='\n'){
            buffer[buffer_index]=0;
            ia_command(buffer);
            buffer_index=0;
        } else {
            buffer[buffer_index++]=c;
            if(buffer_index>=64) buffer_index=63;
        }
    }
}

/* --- Kernel principal ---
 */
void kernel_main(){
    for(int i=0;i<VGA_WIDTH*VGA_HEIGHT;i++) VGA_BUFFER[i]=(0x0F<<8)|' ';
    print("Bienvenue sur AETHERION OS !\n",0x0F);
    print("Initialisation du module IA...\n",0x0A);
    print_ia(reponses_random[0]);

    while(1) ia_input_loop();
}
