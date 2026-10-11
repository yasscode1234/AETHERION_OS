/* link.ld - Script de liaison pour le kernel Aetherion OS */
/* Le kernel est charge a 1 Mio (0x100000) par le bootloader */

ENTRY(kernel_main)

SECTIONS
{
    /* Le kernel commence a 1 Mio */
    . = 0x100000;

    /* Section texte : code executable */
    .text :
    {
        *(.text*)
        *(.rodata*)
    }

    /* Alignement page 4K pour les sections de donnees */
    . = ALIGN(0x1000);

    /* Section donnees initialisees */
    .data :
    {
        *(.data*)
    }

    /* Section BSS (donnees non initialisees) */
    .bss :
    {
        *(.bss*)
        *(COMMON)
    }

    /* Fin du kernel - symbole pour calculer la taille */
    _kernel_end = .;
}
