; boot/bootloader64.asm
; Bootloader minimaliste 64-bit pour test
; Taille : < 512 octets

BITS 16
ORG 0x7C00

start:
    cli                 ; désactiver interruptions
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Petit test : afficher un caractère
    mov ah, 0x0E
    mov al, 'A'
    int 0x10

    hlt                 ; arrêter le CPU

times 510-($-$$) db 0
dw 0xAA55
