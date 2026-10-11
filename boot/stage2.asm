; boot/stage2.asm - Stage 2 Bootloader (logique complète)
; Charge le kernel, configure paging, passe en long mode
; Assemble avec: nasm -f bin boot/stage2.asm -o build/stage2.bin

BITS 16
ORG 0x10000

; ============================================================
; CONSTANTES (injectées au build)
; ============================================================
KERNEL_OFFSET     equ 0x100000
KERNEL_LBA_START  equ 18       ; LBA après MBR(1) + Stage1(1) + Stage2(16) = 18
%include "kernel_sectors.inc"
MAX_CHS_READ      equ 16

; Adresse de chargement kernel = 1MB (0x100000)
; En mode réel, segment:offset 16-bit max = 0xFFFF:0xFFFF = 0x10FFEF
; Pour 0x100000 : utiliser 0xFFFF:0x0010 (0xFFFF0 + 0x10 = 0x100000)
KERNEL_SEG        equ 0xFFFF   ; Segment pour atteindre 1MB via INT 13h AH=42h
KERNEL_OFF        equ 0x0010   ; Offset pour atteindre 1MB (0xFFFF0 + 0x10 = 0x100000)

; Codes de debug Stage 2 (affichés via INT 10h AH=0Eh)
S2_DBG_ENTRY      equ 'A'      ; Entrée Stage 2
S2_DBG_CPU_CHK    equ 'B'      ; Test CPU 64-bit
S2_DBG_CPU_OK     equ 'C'      ; CPU 64-bit OK
S2_DBG_LBA_CHK    equ 'D'      ; Test support LBA
S2_DBG_LBA_OK     equ 'E'      ; LBA supporté
S2_DBG_LBA_READ   equ 'F'      ; Lecture LBA kernel
S2_DBG_LBA_DONE   equ 'G'      ; Lecture LBA OK
S2_DBG_CHS_FB     equ 'H'      ; Fallback CHS
S2_DBG_CHS_LOOP   equ 'I'      ; Boucle lecture CHS
S2_DBG_CHS_DONE   equ 'J'      ; Lecture CHS OK
S2_DBG_PM_GDT     equ 'K'      ; Chargement GDT
S2_DBG_PM_CR0     equ 'L'      ; CR0.PE=1 (protected mode)
S2_DBG_PM_JMP     equ 'M'      ; Saut far vers pm_entry
S2_DBG_PG_CR3     equ 'N'      ; CR3 = PML4
S2_DBG_PG_PML4    equ 'O'      ; PML4 configuré
S2_DBG_PG_PDPT    equ 'P'      ; PDPT configuré
S2_DBG_PG_PD      equ 'Q'      ; PD configuré
S2_DBG_PG_CR4     equ 'R'      ; CR4.PAE=1
S2_DBG_PG_EFER    equ 'S'      ; EFER.LME=1
S2_DBG_PG_CR0_PG  equ 'T'      ; CR0.PG=1 (paging)
S2_DBG_LM_JMP     equ 'U'      ; Saut far vers lm_entry
S2_DBG_LM_READY   equ 'V'      ; Long mode prêt
S2_DBG_KERNEL_JMP equ 'W'      ; Saut vers kernel
S2_ERR_CPU_ID     equ '1'      ; CPUID non supporté
S2_ERR_CPU_LM     equ '2'      ; Long mode non supporté
S2_ERR_LBA        equ '3'      ; LBA non supporté
S2_ERR_DISK       equ '4'      ; Erreur lecture disque
S2_ERR_CHS        equ '5'      ; Erreur CHS

stage2_start:
    cli
    ; IMPORTANT: Stage 2 est chargé à 0x1000:0x0000 (physique 0x10000)
    ; ORG 0x10000 signifie que les labels sont relatifs à 0x10000
    ; Mais le far jump depuis Stage 1 met CS=0x1000, IP=0
    ; Donc DS doit être 0x1000 pour accéder aux données à 0x10000+
    mov ax, 0x1000
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x9000

    ; Récupérer le drive de boot depuis l'adresse fixe en mémoire basse (0x7BFF)
    ; ATTENTION: DS=0x1000, donc [0x7BFF] accéderait à 0x17BFF au lieu de 0x7BFF
    ; Il faut temporairement mettre DS=0 pour lire l'adresse physique 0x7BFF
    push ds
    xor ax, ax
    mov ds, ax
    mov dl, [0x7BFF]
    mov bl, dl          ; Sauvegarder dans BL
    pop ds              ; Restaurer DS=0x1000
    mov [boot_drive], bl ; Écrire dans boot_drive (DS=0x1000)

    ; --- Obtenir la géométrie du disque (INT 13h AH=08h) ---
    call get_drive_params

    ; --- Debug: Stage 2 entry ---
    call dbg_putc
    db S2_DBG_ENTRY

    ; --- 1. CPU 64-bit check ---
    call dbg_putc
    db S2_DBG_CPU_CHK
    call check_cpu64
    jc .cpu_err_lm
    call dbg_putc
    db S2_DBG_CPU_OK

    ; --- 2. LBA support ---
    call dbg_putc
    db S2_DBG_LBA_CHK
    mov ah, 0x41
    mov bx, 0x55AA
    mov dl, [boot_drive]
    int 0x13
    jnc .lba_ok
    cmp bx, 0xAA55
    jne .lba_fail
    test cl, 1
    jz .lba_fail

.lba_ok:
    call dbg_putc
    db S2_DBG_LBA_OK

    ; --- 3. Read kernel via LBA ---
    call dbg_putc
    db S2_DBG_LBA_READ
    ; DS is already 0x1000 (set at stage2_start), DAP is in same segment
    mov si, dap
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jnc .lba_read_ok

    ; Erreur LBA - afficher code erreur AH
    mov al, ah
    call dbg_put_hex
    call dbg_putc
    db S2_ERR_DISK
    jmp .lba_fail

.lba_read_ok:
    call dbg_putc
    db S2_DBG_LBA_DONE
    jmp .kernel_loaded

.lba_fail:
    ; --- Fallback CHS ---
    call dbg_putc
    db S2_DBG_CHS_FB
    call read_chs
    jc .disk_err
    call dbg_putc
    db S2_DBG_CHS_DONE

.kernel_loaded:
    ; --- 4. Protected mode ---
    call dbg_putc
    db S2_DBG_PM_GDT
    lgdt [gdt_ptr]

    call dbg_putc
    db S2_DBG_PM_CR0
    mov eax, cr0
    or al, 1
    mov cr0, eax

    call dbg_putc
    db S2_DBG_PM_JMP
    jmp 0x08:pm_entry

.cpu_err:
    call dbg_putc
    db S2_ERR_CPU_ID
    mov si, msg_cpu_id
    jmp .die
.cpu_err_lm:
    call dbg_putc
    db S2_ERR_CPU_LM
    mov si, msg_cpu_lm
    jmp .die
.disk_err:
    call dbg_putc
    db S2_ERR_DISK
    mov si, msg_disk
.die:
    call print
    hlt

; ============================================================
; ROUTINES 16-BIT
; ============================================================

check_cpu64:
    ; Test si CPUID est supporté (bit 21 EFLAGS modifiable)
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1<<21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    xor eax, ecx
    test eax, 1<<21
    jz .no_cpuid

    ; CPUID supporté - vérifier fonction étendue 0x80000001
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_lmode

    mov eax, 0x80000001
    cpuid
    test edx, 1<<29        ; Bit 29 = Long Mode (LM)
    jz .no_lmode

    clc
    ret

.no_cpuid:
    stc
    ret
.no_lmode:
    stc
    ret

; ============================================================
; Obtenir les paramètres du disque (INT 13h AH=08h)
; Met à jour heads et secs_trk pour lba2chs
; ============================================================
get_drive_params:
    pushad
    mov ah, 0x08
    mov dl, [boot_drive]
    int 0x13
    jc .use_defaults

    ; CH = cylinders low 8 bits (max cylinder = CH + 1)
    ; CL bits 0-5 = sectors per track (max sector = CL & 0x3F)
    ; CL bits 6-7 = cylinders high 2 bits
    ; DH = max head number (heads = DH + 1)
    ; DL = number of drives
    ; ES:DI = pointer to drive parameter table (floppy only)

    ; Sectors per track = CL & 0x3F
    movzx ax, cl
    and ax, 0x3F
    mov [secs_trk], ax

    ; Heads = DH + 1
    movzx ax, dh
    inc ax
    mov [heads], ax

    popad
    ret

.use_defaults:
    ; Valeurs par défaut pour disquette 1.44MB
    mov word [secs_trk], 18
    mov word [heads], 2
    popad
    ret

read_chs:
    mov dword [chs_lba], KERNEL_LBA_START
    mov word [chs_left], KERNEL_SECTORS
    mov word [chs_seg], KERNEL_SEG
    mov word [chs_off], KERNEL_OFF
.loop:
    mov ax, [chs_left]
    test ax, ax
    jz .done

    call dbg_putc
    db S2_DBG_CHS_LOOP

    mov cx, MAX_CHS_READ
    cmp ax, cx
    jge .rd
    mov cx, ax
.rd:
    mov [chs_cnt], cx
    call lba2chs
    mov ah, 0x02
    mov al, [chs_cnt]
    mov ch, [chs_cyl]
    mov cl, [chs_sec]
    mov dh, [chs_hd]
    mov dl, [boot_drive]
    mov bx, [chs_off]
    push word [chs_seg]
    pop es
    int 0x13
    jc .err

    mov ax, [chs_cnt]
    add [chs_lba], ax
    sub [chs_left], ax
    shl ax, 9
    add [chs_off], ax
    jnc .loop

    ; Passer au segment suivant (64 Ko = 0x1000 paragraphs)
    ; ATTENTION: si on dépasse 0xFFFF, le segment wraparound
    mov ax, [chs_seg]
    add ax, 0x1000
    mov [chs_seg], ax
    jmp .loop
.done:
    clc
    ret
.err:
    stc
    ret

lba2chs:
    ; Entrée: [chs_lba] = LBA (32-bit)
    mov eax, [chs_lba]
    xor edx, edx
    div word [secs_trk]
    inc dx
    mov [chs_sec], dl
    xor edx, edx
    div word [heads]
    mov [chs_hd], dl
    mov [chs_cyl], ax
    ret

; ============================================================
; ROUTINES DEBUG 16-BIT
; ============================================================

dbg_putc:
    pop si
    lodsb
    push si
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    ret

dbg_put_hex:
    push ax
    shr al, 4
    call dbg_put_nibble
    pop ax
    call dbg_put_nibble
    ret

dbg_put_nibble:
    and al, 0x0F
    add al, '0'
    cmp al, '9'
    jbe .put
    add al, 7
.put:
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    ret

print:
    lodsb
    test al, al
    jz .ret
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    jmp print
.ret:
    ret

; ============================================================
; DATA 16-BIT
; ============================================================
boot_drive    db 0
chs_lba       dd 0
chs_left      dw 0
chs_seg       dw 0
chs_off       dw 0
chs_cnt       dw 0
chs_cyl       dw 0
chs_hd        db 0
chs_sec       db 0
heads         dw 255
secs_trk      dw 63

msg_cpu_id    db "Stage2: CPUID not supported", 0x0D, 0x0A, 0
msg_cpu_lm    db "Stage2: Long mode not supported", 0x0D, 0x0A, 0
msg_disk      db "Stage2: Disk error", 0x0D, 0x0A, 0

; DAP pour kernel (Disk Address Packet) - Format standard 16 octets pour INT 13h AH=42h
; Format standard INT 13h AH=42h:
; Offset 0:  Size of packet (1 byte) = 0x10
; Offset 1:  Reserved (1 byte) = 0
; Offset 2:  Number of blocks (2 bytes)
; Offset 4:  Buffer offset (2 bytes)
; Offset 6:  Buffer segment (2 bytes)
; Offset 8:  LBA (8 bytes)
; Adresse physique = (segment << 4) + offset = (0xFFFF << 4) + 0x0010 = 0xFFFF0 + 0x10 = 0x100000 (1MB)
align 16
dap:
    db 0x10             ; Taille du paquet (16 octets)
    db 0                ; Réservé
    dw KERNEL_SECTORS   ; Nombre de secteurs à lire (injecté au build)
    dw KERNEL_OFF       ; Offset de destination (0x0010)
    dw KERNEL_SEG       ; Segment de destination (0xFFFF pour 1MB)
    dq KERNEL_LBA_START ; LBA de départ (64-bit)

; GDT (Global Descriptor Table)
; Format: base(32) | limit(20) | flags(12)
; Code64: L=1, D=0, P=1, DPL=0, Type=Execute/Read (0xA)
; Data:   L=0, D=1, P=1, DPL=0, Type=Read/Write (0x2)
; Descriptor format (little-endian):
;   bytes 0-1: limit 0-15
;   bytes 2-4: base 0-23
;   byte 5:    type(4) + S(1) + DPL(2) + P(1)
;   byte 6:    limit 16-19(4) + AVL(1) + L(1) + D/B(1) + G(1)
;   byte 7:    base 24-31
;
; Code64 (L=1, D=0): 0x00AF9A000000FFFF
;   limit=0xFFFFF (4GB with G=1), base=0
;   byte 5: 0x9A = P=1, DPL=0, S=1, Type=0xA (Execute/Read)
;   byte 6: 0xAF = G=1, D=0, L=1, AVL=0, limit_high=0xF
; Data (L=0, D=1): 0x00CF92000000FFFF
;   byte 5: 0x92 = P=1, DPL=0, S=1, Type=0x2 (Read/Write)
;   byte 6: 0xCF = G=1, D=1, L=0, AVL=0, limit_high=0xF
gdt:
    dq 0                                    ; Null descriptor
    dq 0x00AF9A000000FFFF                   ; Code64: base=0, limit=FFFFF, G=1, D=0, L=1, P=1, DPL=0, Type=0xA
    dq 0x00CF92000000FFFF                   ; Data:   base=0, limit=FFFFF, G=1, D=1, L=0, P=1, DPL=0, Type=0x2
gdt_end:
gdt_ptr:
    dw gdt_end - gdt - 1                    ; Limite (taille - 1)
    dq gdt                                  ; Base linéaire (0 en mode réel)

; ============================================================
; MODE PROTÉGÉ 32-BIT
; ============================================================
[BITS 32]
pm_entry:
    ; Configurer les segments de données
    mov ax, 0x10            ; Sélecteur data (GDT[2])
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000        ; Stack au-dessus de l'identité mapping

    ; --- Copier le kernel de 0xFFFF:0x0010 (physique 0x100000) vers 0x100000 linéaire ---
    ; Le BIOS a chargé le kernel à 0xFFFF:0x0010 = physique 0x100000
    ; En mode protégé avec identity mapping, c'est déjà à la bonne adresse linéaire
    ; Mais on doit s'assurer que l'identity mapping couvre 0x100000
    ; (ce que fait le code de pagination ci-dessous)

    ; --- Pagination: Identity map 16MB (8x 2MB pages) ---
    ; Couvre 0x000000 - 0x1000000 (kernel à 0x100000 + marge pour croissance)
    call dbg_putc32
    db S2_DBG_PG_CR3
    mov edi, 0x1000         ; PML4 à 0x1000 (aligné 4K)
    mov cr3, edi
    xor eax, eax
    mov ecx, 4096           ; Vider 4K (une page)
    rep stosd

    call dbg_putc32
    db S2_DBG_PG_PML4
    mov edi, 0x1000
    mov dword [edi], 0x2003      ; PML4[0] -> PDPT à 0x2000 (P=1, RW=1, US=0)
    mov dword [edi+4], 0
    mov dword [edi+8], 0         ; PML4[1] = 0 (pas d'adresses > 4GB)
    mov dword [edi+12], 0

    call dbg_putc32
    db S2_DBG_PG_PDPT
    mov edi, 0x2000         ; PDPT à 0x2000
    mov dword [edi], 0x3003      ; PDPT[0] -> PD0 à 0x3000 (0-4MB)
    mov dword [edi+4], 0x4003    ; PDPT[1] -> PD1 à 0x4000 (4-8MB)
    mov dword [edi+8], 0x5003    ; PDPT[2] -> PD2 à 0x5000 (8-12MB)
    mov dword [edi+12], 0x6003   ; PDPT[3] -> PD3 à 0x6000 (12-16MB)

    call dbg_putc32
    db S2_DBG_PG_PD
    ; PD0 à 0x3000 : 0-4MB (2 pages de 2MB)
    mov edi, 0x3000
    mov dword [edi], 0x00000083  ; PD0[0] -> 2MB page @ 0x000000 (P=1, RW=1, US=0, PS=1, G=1)
    mov dword [edi+4], 0x200083  ; PD0[1] -> 2MB page @ 0x200000
    mov dword [edi+8], 0
    mov dword [edi+12], 0

    ; PD1 à 0x4000 : 4-8MB
    mov edi, 0x4000
    mov dword [edi], 0x400083    ; PD1[0] -> 2MB page @ 0x400000
    mov dword [edi+4], 0x600083  ; PD1[1] -> 2MB page @ 0x600000
    mov dword [edi+8], 0
    mov dword [edi+12], 0

    ; PD2 à 0x5000 : 8-12MB
    mov edi, 0x5000
    mov dword [edi], 0x800083    ; PD2[0] -> 2MB page @ 0x800000
    mov dword [edi+4], 0xA00083  ; PD2[1] -> 2MB page @ 0xA00000
    mov dword [edi+8], 0
    mov dword [edi+12], 0

    ; PD3 à 0x6000 : 12-16MB
    mov edi, 0x6000
    mov dword [edi], 0xC00083    ; PD3[0] -> 2MB page @ 0xC00000
    mov dword [edi+4], 0xE00083  ; PD3[1] -> 2MB page @ 0xE00000
    mov dword [edi+8], 0
    mov dword [edi+12], 0

    call dbg_putc32
    db S2_DBG_PG_CR4
    mov eax, cr4
    or eax, 1<<5            ; CR4.PAE = 1 (Physical Address Extension)
    mov cr4, eax

    call dbg_putc32
    db S2_DBG_PG_EFER
    mov ecx, 0xC0000080     ; IA32_EFER MSR
    rdmsr
    or eax, 1<<8            ; EFER.LME = 1 (Long Mode Enable)
    wrmsr

    call dbg_putc32
    db S2_DBG_PG_CR0_PG
    mov eax, cr0
    or eax, 1<<31           ; CR0.PG = 1 (Enable Paging)
    mov cr0, eax

    call dbg_putc32
    db S2_DBG_LM_JMP
    jmp 0x08:lm_entry       ; Far jump vers code64 selector (GDT[1])

; ============================================================
; MODE 64-BIT (LONG MODE)
; ============================================================
[BITS 64]
lm_entry:
    ; Configurer segments 64-bit (data selectors = 0 en long mode)
    mov ax, 0x10            ; Data selector
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov rsp, 0x90000        ; Stack 64-bit

    call dbg_putc64
    db S2_DBG_LM_READY

    ; Saut vers le kernel à 0x100000 (identity mapped)
    call dbg_putc64
    db S2_DBG_KERNEL_JMP
    jmp KERNEL_OFFSET

; ============================================================
; ROUTINES DEBUG 32-BIT / 64-BIT
; ============================================================

[BITS 32]
dbg_putc32:
    ; En mode protégé, on écrit directement en mémoire vidéo (0xB8000)
    ; Lit le caractère après le CALL (comme dbg_putc 16-bit)
    push eax
    push edi
    push esi
    mov esi, [esp + 12]       ; Adresse de retour (après CALL)
    lodsb                     ; Charge le caractère de debug
    mov [esp + 12], esi       ; Met à jour l'adresse de retour (après le DB)
    mov edi, 0xB8000 + 24*160 ; Ligne 24, colonne 0
    mov ah, 0x0F              ; Blanc sur noir
    stosw
    pop esi
    pop edi
    pop eax
    ret

[BITS 64]
dbg_putc64:
    push rax
    push rdi
    push rsi
    mov rsi, [rsp + 24]       ; Adresse de retour (après CALL)
    lodsb                     ; Charge le caractère de debug
    mov [rsp + 24], rsi       ; Met à jour l'adresse de retour (après le DB)
    mov rdi, 0xB8000 + 24*160 ; Ligne 24, colonne 0
    mov ah, 0x0F              ; Blanc sur noir
    stosw
    pop rsi
    pop rdi
    pop rax
    ret
