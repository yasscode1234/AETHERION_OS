/* Linker script AETHERION OS 64-bit (GAS)
 * Place tout à 0x100000 et démarre à _start (bootloader.S)
 */
OUTPUT_FORMAT("elf64-x86-64")
ENTRY(_start)

SECTIONS
{
  /* Adresse de chargement en mémoire */
  . = 0x100000;

  .text : {
    *(.text*)
  }

  .rodata : {
    *(.rodata*)
  }

  .data : {
    *(.data*)
  }

  .bss : {
    *(COMMON)
    *(.bss*)
  }

  /DISCARD/ : {
    *(.comment)
    *(.note*)
  }
}
