; boot/bootloader64.asm
BITS 16
ORG 0x7C00

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; GDT minimal pour long mode
    lgdt [gdt_descriptor]

    ; Activer Protected Mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Far jump pour init PM
    jmp 0x08:init_pm

[BITS 32]
init_pm:
    ; Setup simple du segment code pour long mode
    mov ax, 0x08
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x90000

    ; Passer en 64-bit
    ; simple long jump à kernel_entry64
    jmp 0x08:kernel_entry64

[BITS 64]
kernel_entry64:
    hlt

; -------------------------------
CODE_SEG    equ 0x08

gdt_start:
    dq 0x0                  ; NULL
    dq 0x00AF9A000000FFFF   ; code segment 64-bit
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dq gdt_start

times 510-($-$$) db 0
dw 0xAA55
