// kernel.c
#include "kernel.h"
#include "vga.h"
#include "keyboard.h"
#include "ia.h"
#include "types.h"
#include "system.h"
#include "io.h"
#include <string.h>

// Config
#define BUFFER_SIZE 128
#define USERNAME_MAX 32
#define WELCOME_X 51
#define WELCOME_Y 2

static char console_buffer[BUFFER_SIZE];
static int console_index = 0;

unsigned char curseur_colors[] = {1,4,6,3};
int color_index = 0;
int cursor_x = 2, cursor_y = 2;

char username[USERNAME_MAX];

// affiche lettre par lettre et animate le curseur
void ia_print_letter_by_letter(const char* msg, int x, int y, uint8_t color) {
    int px = x;
    int py = y;
    for(int i=0; msg[i]; i++) {
        char ch = msg[i];
        vga_draw_text(px, py, &ch, color);
        px++;
        if(px > 79) { px = 0; py++; }
        // petit delay visuel
        for(volatile int d=0; d<30000; d++);
        vga_draw_cursor(cursor_x, cursor_y, curseur_colors[color_index]);
        color_index = (color_index + 1) % 4;
    }
}

// dessine le bureau
void draw_desktop(void) {
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

// met à jour curseur (couleur cyclique)
void update_cursor(void) {
    if(cursor_x < 0) cursor_x = 0;
    if(cursor_x > 79) cursor_x = 79;
    if(cursor_y < 0) cursor_y = 0;
    if(cursor_y > 24) cursor_y = 24;
    vga_draw_cursor(cursor_x, cursor_y, curseur_colors[color_index]);
    color_index = (color_index + 1) % 4;
}

// demande et persiste le nom utilisateur via fs
void ensure_username(void) {
    // teste si username déjà présent dans FS
    int r = fs_read_file("username", username, USERNAME_MAX);
    if(r > 0) {
        // déjà présent; s'assurer terminaisons
        username[USERNAME_MAX-1] = 0;
        return;
    }

    // sinon, demande et stocke
    vga_draw_text(2,2,"Entrez votre nom: ",15);
    cursor_x = 20; cursor_y = 2;
    int idx = 0;
    while(1) {
        update_cursor();
        char c = keyboard_get_char();
        if(c) {
            if(c == '\n' || idx >= USERNAME_MAX-1) {
                username[idx] = 0;
                fs_write_file("username", username, (uint32_t)(idx+1));
                break;
            } else {
                username[idx++] = c;
                // afficher caractère
                vga_draw_text(20 + idx, 2, &c, 15);
                cursor_x++;
            }
        }
    }
}

// kernel principal
void kernel_main(void) {
    // init hardware abstraction
    vga_init();
    keyboard_init();

    // initialise IO (disk AHCI/VM)
    if(io_init() != IO_OK) {
        vga_draw_text(0, 0, "IO init failed", 12);
        for(;;) __asm__("hlt");
    }

    // initialise FS (charge si présent)
    fs_init();

    // draw UI and get username
    draw_desktop();
    ensure_username();

    // bienvenue
    char welcome[80];
    // utilise sprintf (impl fourni dans string.c)
    sprintf(welcome, "Bonjour %s !", username);
    ia_print_letter_by_letter(welcome, WELCOME_X, WELCOME_Y, 15);

    // boucle principale console
    while(1) {
        update_cursor();
        char c = keyboard_get_char();
        if(c) {
            if(c == '\n') {
                console_buffer[console_index] = 0;
                ia_execute_command(console_buffer);
                console_index = 0;
                cursor_x = 2; cursor_y++;
                if(cursor_y > 24) cursor_y = 24;
            } else if(c == 0xE0) {
                int dir = keyboard_arrow();
                if(dir != 0) ia_history_nav(dir, console_buffer, &console_index);
            } else {
                if(console_index < BUFFER_SIZE-1) {
                    console_buffer[console_index++] = c;
                }
                if(cursor_x < 79) cursor_x++;
            }
        }
    }
}
