#include "kernel.h"
#include "vga.h"
#include "keyboard.h"
#include "ia.h"
#include "types.h"
#include <string.h>
#include "system.h"

#define BUFFER_SIZE 64
#define MAX_CURSOR_X 79
#define MAX_CURSOR_Y 24

static char buffer[BUFFER_SIZE];
static int buffer_index = 0;

unsigned char curseur_colors[] = {1,4,6,3};
int color_index = 0;
int cursor_x = 2, cursor_y = 2;

void ia_print_letter_by_letter(const char* msg, int x, int y, uint8_t color) {
    for(int i=0; msg[i]; i++) {
        int draw_x = x + i;
        int draw_y = y;
        if(draw_x > MAX_CURSOR_X) { draw_x = MAX_CURSOR_X; draw_y++; }
        if(draw_y > MAX_CURSOR_Y) break;
        vga_draw_text(draw_x, draw_y, &msg[i], color);
        for(volatile int d=0; d<30000; d++);
        vga_draw_cursor(cursor_x, cursor_y, curseur_colors[color_index]);
        color_index = (color_index + 1) % 4;
    }
}

void draw_desktop() {
    vga_clear_screen();
    vga_fade_in(0,0,50,20,1,9);
    vga_draw_border(0,0,50,20,15);
    vga_draw_text(2,0,"COMMANDES",15);

    vga_fade_in(50,0,30,20,3,11);
    vga_draw_border(50,0,30,20,15);
    vga_draw_text(51,0,"IA",15);

    vga_draw_gradient(0,20,80,5,5,13);
    vga_draw_text(1,20,"AETHERION OS v0.1 | 16:00 | Parametres",15);
}

void update_cursor() {
    if(cursor_x > MAX_CURSOR_X) cursor_x = MAX_CURSOR_X;
    if(cursor_y > MAX_CURSOR_Y) cursor_y = MAX_CURSOR_Y;
    vga_draw_cursor(cursor_x, cursor_y, curseur_colors[color_index]);
    color_index = (color_index + 1) % 4;
}

void kernel_main() {
    vga_init();
    draw_desktop();
    ia_print_letter_by_letter("Bonjour yasscode !",51,2,15);
    keyboard_init();

    while(1) {
        update_cursor();
        char c = keyboard_get_char();
        if(c) {
            if(c == '\n') {
                buffer[buffer_index] = 0;
                ia_execute_command(buffer);
                buffer_index = 0;
                cursor_x = 2; cursor_y++;
                if(cursor_y > MAX_CURSOR_Y) cursor_y = MAX_CURSOR_Y;
            } else if(c == 0xE0) {
                int dir = keyboard_arrow();
                if(dir != 0) ia_history_nav(dir, buffer, &buffer_index);
            } else {
                if(buffer_index < BUFFER_SIZE - 1) {
                    buffer[buffer_index++] = c;
                }
                if(cursor_x < MAX_CURSOR_X) cursor_x++;
            }
        }
    }
}
