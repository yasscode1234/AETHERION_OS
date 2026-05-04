; AETHERION OS - Bootloader
; Copyright (C) 2026 yasscode
; Licensed under GPLv2

[BITS 16]
[ORG 0x7C00]

start:
    cli

    ; (1) Activer la ligne A20 (indispensable en vrai hardware)
    ; >>> À implémenter proprement (contrôleur clavier ou BIOS) <<<
    ; call enable_a20

    ; (2) Charger la GDT 16 bits (descripteur 6 octets : limit(2) + base(4))
    lgdt [gdt_descriptor]

    ; (3) Passer en mode protégé
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp CODE_SEG:protected_mode_entry   ; far jump pour recharger CS

[BITS 32]
protected_mode_entry:
    ; (4) Initialiser les segments
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Stack 32 bits provisoire
    mov esp, 0x90000

    ; (5) Préparer le paging + long mode
    ; >>> ICI il manque encore les tables de pages (PML4, PDPT, PD, PT) <<<
    ; Tu dois :
    ;   - allouer une PML4 alignée (ex: 0x00100000)
    ;   - remplir PML4 -> PDPT -> PD -> PT en identity mapping au moins pour 0–1 MiB
    ;   - mettre l’adresse de la PML4 dans CR3

    ; Exemple (à adapter une fois les tables créées) :
    ;   mov eax, PML4_ADDR
    ;   mov cr3, eax

    ; (6) Activer LME dans EFER (MSR 0xC0000080)
    mov ecx, 0xC0000080
    rdmsr
    or eax, 0x00000100          ; LME = 1
    wrmsr

    ; (7) Activer PAE dans CR4
    mov eax, cr4
    or eax, 0x20                ; PAE = 1
    mov cr4, eax

    ; (8) Activer le paging dans CR0
    mov eax, cr0
    or eax, 0x80000000          ; PG = 1
    mov cr0, eax

    ; (9) Saut en long mode (code 64 bits)
    jmp CODE_SEG:long_mode_entry

[BITS 64]
long_mode_entry:
    ; Stack 64 bits
    mov rsp, 0x100000

    extern kernel_main
    call kernel_main

hang:
    hlt
    jmp hang

; =========================
; GDT
; =========================

gdt_start:
    dq 0x0000000000000000       ; Null
    dq 0x00AF9A000000FFFF       ; Code 64 bits
    dq 0x00AF92000000FFFF       ; Data 64 bits
gdt_end:

; Descripteur GDT : 6 octets (limit 2 + base 4)
gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start                ; ⚠ corrigé : dd et pas dq

CODE_SEG equ 0x08
DATA_SEG equ 0x10

times 510-($-$$) db 0
dw 0xAA55
