; AETHERION OS - Bootloader
; Copyright (C) 2026 yasscode
; Licensed under GPLv2

[BITS 16]
[ORG 0x7C00]

STAGE2_ADDR    equ 0x7E00
STAGE2_SECTORS equ 32

KERNEL_ADDR    equ 0x10000
KERNEL_SECTORS equ 64
KERNEL_LBA     equ 33

CODE32_SEG equ 0x08
DATA_SEG   equ 0x10
CODE64_SEG equ 0x18

PML4_ADDR equ 0x1000
PDPT_ADDR equ 0x2000
PD_ADDR   equ 0x3000

; =========================
; Stage 1, chargé par le BIOS à 0x7C00
; =========================

stage1_start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [boot_drive_stage1], dl

    ; Charger le stage 2 depuis LBA 1 vers 0x7E00
    mov si, stage2_dap
    mov ah, 0x42
    mov dl, [boot_drive_stage1]
    int 0x13
    jc disk_error_stage1

    mov dl, [boot_drive_stage1]
    jmp 0x0000:STAGE2_ADDR

disk_error_stage1:
    cli
.hang:
    hlt
    jmp .hang

boot_drive_stage1:
    db 0

stage2_dap:
    db 0x10
    db 0
    dw STAGE2_SECTORS
    dw STAGE2_ADDR
    dw 0x0000
    dq 1

times 510 - ($ - $$) db 0
dw 0xAA55

; =========================
; Stage 2, chargé à 0x7E00
; =========================

[BITS 16]
[ORG STAGE2_ADDR]          ; 🔥 correction clé : le stage 2 est assemblé pour 0x7E00

stage2_start:
    cli

    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    mov [boot_drive_stage2], dl

    call enable_a20

    ; Charger le noyau depuis LBA 33 vers 0x10000
    mov si, kernel_dap
    mov ah, 0x42
    mov dl, [boot_drive_stage2]
    int 0x13
    jc disk_error_stage2

    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp CODE32_SEG:protected_mode_entry

enable_a20:
    in al, 0x92
    or al, 00000010b
    and al, 11111110b
    out 0x92, al
    ret

disk_error_stage2:
    cli
.hang:
    hlt
    jmp .hang

boot_drive_stage2:
    db 0

kernel_dap:
    db 0x10
    db 0
    dw KERNEL_SECTORS
    dw 0x0000
    dw 0x1000
    dq KERNEL_LBA

[BITS 32]
protected_mode_entry:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, 0x90000

    ; Nettoyer PML4, PDPT et PD
    mov edi, PML4_ADDR
    xor eax, eax
    mov ecx, (4096 * 3) / 4
    rep stosd

    ; PML4[0] -> PDPT
    mov dword [PML4_ADDR], PDPT_ADDR | 0x03
    mov dword [PML4_ADDR + 4], 0

    ; PDPT[0] -> PD
    mov dword [PDPT_ADDR], PD_ADDR | 0x03
    mov dword [PDPT_ADDR + 4], 0

    ; PD[0] -> page 2 MiB identity mapped
    mov dword [PD_ADDR], 0x00000083
    mov dword [PD_ADDR + 4], 0

    mov eax, PML4_ADDR
    mov cr3, eax

    ; Activer PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Activer Long Mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Activer paging
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    jmp CODE64_SEG:long_mode_entry

[BITS 64]
long_mode_entry:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov ss, ax

    mov rsp, 0x90000

    mov rax, KERNEL_ADDR
    call rax

.hang:
    hlt
    jmp .hang

; =========================
; GDT
; =========================

gdt_start:
    dq 0x0000000000000000       ; Null
    dq 0x00CF9A000000FFFF       ; Code 32 bits
    dq 0x00CF92000000FFFF       ; Data
    dq 0x00AF9A000000FFFF       ; Code 64 bits
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start                ; base linéaire correcte (0x7E00 + offset)

; Padding : stage 1 + stage 2 = 33 secteurs
times (33 * 512) - ($ - $$) db 0
