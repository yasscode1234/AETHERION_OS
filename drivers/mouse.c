#include "mouse.h"
#include "io.h"
static int mx=40,my=12,btn=0;
void mouse_init(void){
    // si PS/2 non impl -> stub
    mx=40; my=12; btn=0;
}
int mouse_get_event(int* dx,int* dy,int* buttons){
    *dx = 0; *dy = 0; *buttons = btn;
    return 0; // 0 pas d'événement, 1 évènement
}
