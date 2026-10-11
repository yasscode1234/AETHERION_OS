#include "kernel.h"
#include "vga.h"
#include "keyboard.h"
#include "disk.h"
#include "ata.h"
#include "ahci.h"
#include "pci.h"
#include "string.h"
#include "stdlib.h"

/* ---- Point d'entree du kernel (appele par le bootloader) ---- */
void kernel_main(void) {
    vga_init();

    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_write("=====================================\n");
    vga_write("   AETHERION OS - Kernel v0.3\n");
    vga_write("=====================================\n\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_write("Kernel initialise avec succes.\n");
    vga_write("Bienvenue dans Aetherion OS !\n\n");

    /* Initialiser le clavier */
    keyboard_init();

    /* Initialiser PCI */
    pci_init();

    /* Initialiser les disques */
    disk_init();
    ata_init();
    ahci_init();

    /* Lister les disques */
    disk_list();

    /* Lister les peripheriques PCI */
    pci_list_devices();

    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_write("Systeme pret. Tapez 'help' pour les commandes.\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_write("aetherion> ");

    /* Buffer de commande */
    char cmd_buf[256];
    size_t cmd_len = 0;

    /* Boucle principale : shell simple */
    for (;;) {
        char c = keyboard_getchar();

        if (c == '\n' || c == '\r') {
            vga_putchar('\n');
            cmd_buf[cmd_len] = 0;

            /* Traiter la commande */
            if (cmd_len > 0) {
                if (strcmp(cmd_buf, "help") == 0) {
                    vga_write("Commandes disponibles:\n");
                    vga_write("  help     - Afficher cette aide\n");
                    vga_write("  clear    - Effacer l'ecran\n");
                    vga_write("  disks    - Lister les disques\n");
                    vga_write("  pci      - Lister les peripheriques PCI\n");
                    vga_write("  read <disk> <lba> <count> - Lire secteurs\n");
                    vga_write("  echo <text> - Afficher du texte\n");
                    vga_write("  reboot   - Redemarrer (triple fault)\n");
                }
                else if (strcmp(cmd_buf, "clear") == 0) {
                    vga_clear();
                }
                else if (strcmp(cmd_buf, "disks") == 0) {
                    disk_list();
                }
                else if (strcmp(cmd_buf, "pci") == 0) {
                    pci_list_devices();
                }
                else if (strncmp(cmd_buf, "read ", 5) == 0) {
                    int disk_idx, lba, count;
                    /* sscanf minimal - parse manuel */
                    char* p = cmd_buf + 5;
                    while (*p == ' ') p++;
                    disk_idx = atoi(p);
                    while (*p >= '0' && *p <= '9') p++;
                    while (*p == ' ') p++;
                    lba = atoi(p);
                    while (*p >= '0' && *p <= '9') p++;
                    while (*p == ' ') p++;
                    count = atoi(p);

                    disk_t* d = disk_get(disk_idx);
                    if (d) {
                        void* buf = malloc(count * 512);
                        if (buf && disk_read(d, lba, count, buf) == 0) {
                            vga_printf("Lu %d secteurs du disque %d (LBA %d)\n", count, disk_idx, lba);
                            /* Afficher hex dump des premiers 256 octets */
                            u8* data = (u8*)buf;
                            for (int i = 0; i < 256 && i < count * 512; i++) {
                                if (i % 16 == 0) vga_printf("\n%04x: ", i);
                                vga_printf("%02x ", data[i]);
                            }
                            vga_putchar('\n');
                        } else {
                            vga_write("Erreur lecture\n");
                        }
                        free(buf);
                    } else {
                        vga_write("Disque invalide\n");
                    }
                }
                else if (strncmp(cmd_buf, "echo ", 5) == 0) {
                    vga_write(cmd_buf + 5);
                    vga_putchar('\n');
                }
                else if (strcmp(cmd_buf, "reboot") == 0) {
                    vga_write("Redemarrage...\n");
                    __asm__ volatile ("cli; hlt");
                }
                else {
                    vga_printf("Commande inconnue: %s\n", cmd_buf);
                }
            }

            cmd_len = 0;
            vga_write("aetherion> ");
        }
        else if (c == '\b') {
            if (cmd_len > 0) {
                cmd_len--;
                vga_putchar('\b');
            }
        }
        else if (c >= 32 && c < 127 && cmd_len < sizeof(cmd_buf) - 1) {
            cmd_buf[cmd_len++] = c;
            vga_putchar(c);
        }
    }
}
