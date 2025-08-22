; AETHERION OS - Bootloader
; Copyright (C) 2025 yasscode
; Licensed under GPLv2

[BITS 16]
[ORG 0x7C00]

start:
    cli                     ; Désactive les interruptions
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp CODE_SEG:protected_mode_entry

[BITS 32]
protected_mode_entry:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    mov ecx, 0xC0000080
    rdmsr
    or eax, 0x00000100
    wrmsr

    mov eax, cr4
    or eax, 0x20
    mov cr4, eax

    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    jmp 0x08:long_mode_entry

[BITS 64]
long_mode_entry:
    mov rsp, 0x100000
    extern kernel_main
    call kernel_main

hang:
    hlt
    jmp hang

gdt_start:
    dq 0x0000000000000000
    dq 0x00AF9A000000FFFF
    dq 0x00AF92000000FFFF
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dq gdt_start

CODE_SEG equ 0x08
DATA_SEG equ 0x10

times 510-($-$$) db 0
dw 0xAA55
