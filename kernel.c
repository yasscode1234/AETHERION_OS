/*
 * AETHERION OS - Kernel
 * Copyright (C) 2025 yasscode
 * Licensed under GPLv2
 */

/* --- Types simples ---
 * On définit nos propres types pour éviter stdint.h
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

void put_char(char c, uint8_t color) {
    if (c == '\n') { cursor_x = 0; cursor_y++; }
    else {
        const int index = cursor_y * VGA_WIDTH + cursor_x;
        VGA_BUFFER[index] = ((uint16_t)color << 8) | c;
        cursor_x++;
        if(cursor_x >= 50){ cursor_x = 0; cursor_y++; }
    }
}

void print(const char* str, uint8_t color) {
    while(*str) put_char(*str++, color);
}

/* --- Zone IA ---
 */
void print_ia(const char* str) {
    int start_x = 50;
    int start_y = 2;
    for(int i=0; str[i]!='\0'; i++){
        int index = start_y*VGA_WIDTH + start_x + i;
        VGA_BUFFER[index] = ((uint16_t)0x0A << 8) | str[i];
    }
}

/* --- Fonctions simples pour chaînes ---
 */
int strcmp(const char* a, const char* b){
    while(*a && (*a==*b)){ a++; b++; }
    return *(unsigned char*)a - *(unsigned char*)b;
}

int strncmp(const char* a, const char* b, int n){
    for(int i=0;i<n;i++){
        if(a[i]!=b[i] || a[i]==0 || b[i]==0) return a[i]-b[i];
    }
    return 0;
}

int strlen(const char* s){
    int i=0; while(s[i]) i++; return i;
}

/* --- Conversion int -> ASCII basique ---
 */
void itoa(int value, char* str){
    char buf[10]; int i=0;
    if(value==0){ str[0]='0'; str[1]=0; return; }
    while(value>0){
        buf[i++] = '0'+(value%10);
        value/=10;
    }
    int j=0; while(i>0) str[j++] = buf[--i];
    str[j]=0;
}

/* --- Système de paramètres ---
 */
typedef struct {
    uint8_t brightness;
    uint8_t volume;
    uint8_t language; // 0=FR,1=EN
} system_params_t;

system_params_t sys_params = {10,5,0};

/* --- IA et réponses ---
 */
const char* reponses_ia[] = {
    "Bonjour, je suis votre assistant AETHERION OS.",
    "Tape 'help' pour la liste des commandes.",
    "Je peux gerer les parametres et les mises a jour."
};

void repondre_ia(int index){
    if(index<0 || index>=3) return;
    print_ia(reponses_ia[index]);
}

/* --- Commandes ---
 */
char buffer[64];
int buffer_index=0;

void ia_command(const char* cmd){
    char tmp[32];
    if(strcmp(cmd,"help")==0){
        print_ia("Commandes: help, update, settings, set, show");
    }
    else if(strcmp(cmd,"update")==0){
        print_ia("Recherche mise a jour...");
        for(volatile int i=0;i<1000000;i++); // simulation
        print_ia("Mise a jour disponible: AETHERION OS v0.1");
    }
    else if(strcmp(cmd,"settings")==0){
        char s[64];
        char t[10];
        itoa(sys_params.brightness,t);
        int k=0; while(t[k]) s[k]=t[k++]; s[k++]=' ';
        itoa(sys_params.volume,t); int m=0; while(t[m]) s[k++]=t[m++]; s[k++]=' ';
        itoa(sys_params.language,t); int n=0; while(t[n]) s[k++]=t[n++]; s[k]=0;
        print_ia(s);
    }
    else if(strncmp(cmd,"set ",4)==0){
        int v=0; char p[16]; int i=0,j=0;
        // copie paramètre
        while(cmd[4+i]!=' ' && cmd[4+i]!=0){ p[i]=cmd[4+i]; i++; } p[i]=0;
        // copie valeur
        j=0; i++; while(cmd[4+i]!='\0'){ v=v*10+(cmd[4+i]-'0'); i++; }
        if(strcmp(p,"brightness")==0) sys_params.brightness=v;
        else if(strcmp(p,"volume")==0) sys_params.volume=v;
        else if(strcmp(p,"language")==0) sys_params.language=v;
        print_ia("Parametre modifie.");
    }
    else if(strcmp(cmd,"show")==0){
        char s[64];
        char t[10];
        int k=0;
        itoa(sys_params.brightness,t); int m=0; while(t[m]) s[k++]=t[m++];
        s[k++]=' ';
        itoa(sys_params.volume,t); m=0; while(t[m]) s[k++]=t[m++];
        s[k++]=' ';
        itoa(sys_params.language,t); m=0; while(t[m]) s[k++]=t[m++];
        s[k]=0;
        print_ia(s);
    }
    else print_ia("Commande inconnue.");
}

/* --- Lecture clavier ---
 */
char read_key(){ return 0; } // simulation, à remplacer par code réel

void ia_input_loop(){
    char c;
    while((c=read_key())!=0){
        if(c=='\n'){
            buffer[buffer_index]=0;
            ia_command(buffer);
            buffer_index=0;
        }else{
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
    repondre_ia(0);

    while(1) ia_input_loop();
}
