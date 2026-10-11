; boot/stage1.asm - Stage 1 Bootloader (512 octets)
; Charge Stage 2 depuis le disque et saute vers lui
; Assemble avec: nasm -f bin boot/stage1.asm -o build/stage1.bin

BITS 16
ORG 0x7C00

; Configuration
STAGE2_LBA_START  equ 2        ; LBA où commence Stage 2 (après MBR + Stage 1)
STAGE2_SECTORS    equ 16       ; Nombre de secteurs pour Stage 2 (8 Ko)
STAGE2_SEG        equ 0x1000   ; Segment de chargement Stage 2 (0x10000)
STAGE2_OFF        equ 0x0000   ; Offset de chargement Stage 2

; Codes de debug (affichés via INT 10h AH=0Eh)
DBG_START         equ '1'      ; Démarrage Stage 1
DBG_A20_OK        equ '2'      ; A20 activé
DBG_LBA_CHK       equ '3'      ; Test support LBA
DBG_LBA_OK        equ '4'      ; LBA supporté
DBG_LBA_READ      equ '5'      ; Lecture LBA Stage 2
DBG_LBA_DONE      equ '6'      ; Lecture LBA OK
DBG_CHS_FB        equ '7'      ; Fallback CHS
DBG_CHS_DONE      equ '8'      ; Lecture CHS OK
DBG_JMP_ST2       equ '9'      ; Saut vers Stage 2
DBG_ERR_DISK      equ 'E'      ; Erreur disque
DBG_ERR_LBA       equ 'L'      ; LBA non supporté
DBG_ERR_A20       equ 'A'      ; A20 failed (rare)

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Sauvegarder le drive de boot (DL fourni par BIOS) AVANT d'utiliser la pile
    mov [boot_drive], dl

    ; --- Debug: Stage 1 started ---
    call dbg_putc
    db DBG_START

    ; --- 1. Activer A20 (Fast A20 gate) ---
    in al, 0x92
    test al, 2
    jnz .a20_already_on
    or al, 2
    out 0x92, al
    in al, 0x92
    test al, 2
    jz .a20_failed
.a20_already_on:
    call dbg_putc
    db DBG_A20_OK

    ; --- 2. Obtenir la géométrie du disque (INT 13h AH=08h) ---
    call get_drive_params

    ; --- 3. Vérifier support LBA (INT 13h AH=41h) ---
    call dbg_putc
    db DBG_LBA_CHK
    mov ah, 0x41
    mov bx, 0x55AA
    mov dl, [boot_drive]
    int 0x13
    jc .use_chs
    cmp bx, 0xAA55
    jne .use_chs
    test cl, 1
    jz .use_chs

    ; LBA supporté
    call dbg_putc
    db DBG_LBA_OK

    ; --- 4. Lire Stage 2 via LBA (INT 13h AH=42h) ---
    call dbg_putc
    db DBG_LBA_READ
    ; Récupérer le drive AVANT de changer DS (boot_drive est accessible avec DS=0)
    mov dl, [boot_drive]
    push ds
    mov ax, 0x07C0        ; Segment de code où se trouve le DAP (0x07C0:0 = 0x7C00)
    mov ds, ax
    mov si, dap
    mov ah, 0x42
    int 0x13
    pop ds
    jnc .lba_read_ok

    ; Erreur LBA - afficher code erreur AH
    mov al, ah
    call dbg_put_hex
    call dbg_putc
    db DBG_ERR_DISK
    jmp .use_chs

.lba_read_ok:
    call dbg_putc
    db DBG_LBA_DONE
    jmp .stage2_loaded

.use_chs:
    call dbg_putc
    db DBG_CHS_FB
    ; Fallback CHS avec géométrie détectée
    call read_chs_fallback
    jc .disk_error

    call dbg_putc
    db DBG_CHS_DONE

.stage2_loaded:
    ; --- 4. Sauter vers Stage 2 ---
    ; Transmettre le numéro de drive de boot à Stage 2 via adresse fixe en mémoire basse (0x7BFF)
    ; IMPORTANT: écrire avec DS=0 pour que ce soit à l'adresse physique 0x7BFF
    ; FAIT AVANT d'utiliser la pile pour éviter la corruption (SP=0x7C00)
    push ds
    xor ax, ax
    mov ds, ax
    mov byte [0x7BFF], dl
    pop ds
    
    ; Debug: saut vers Stage 2
    call dbg_putc
    db DBG_JMP_ST2
    
    ; Far jump explicite vers Stage 2 (0x1000:0x0000 = 0x10000)
    ; Syntaxe NASM explicite pour far jump
    jmp 0x1000:0x0000

.disk_error:
    mov si, msg_disk
    call print
    hlt

.a20_failed:
    call dbg_putc
    db DBG_ERR_A20
    mov si, msg_a20
    call print
    hlt

; ============================================================
; ROUTINES DEBUG
; ============================================================

; Affiche un caractère de debug (passé en DB après CALL)
dbg_putc:
    pop si              ; Récupère l'adresse de retour (qui pointe sur le DB)
    lodsb               ; Charge le caractère
    push si             ; Remet l'adresse suivante comme retour
    mov ah, 0x0E
    mov bx, 0x0007      ; Page 0, attribut blanc sur noir
    int 0x10
    ret

; Affiche AL en hexa (2 chiffres)
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
    add al, 7           ; 'A' - '9' - 1
.put:
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    ret

; Affiche une chaîne terminée par 0 (SI = adresse)
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
; Obtenir les paramètres du disque (INT 13h AH=08h)
; Met à jour heads et secs_trk pour read_chs_fallback
; ============================================================
get_drive_params:
    push ax
    push bx
    push cx
    push dx
    push es
    push di
    mov ah, 0x08
    mov dl, [boot_drive]
    int 0x13
    jc .use_defaults

    ; CH = cylinders low 8 bits
    ; CL bits 0-5 = sectors per track
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

    pop di
    pop es
    pop dx
    pop cx
    pop bx
    pop ax
    ret

.use_defaults:
    ; Valeurs par défaut pour disquette 1.44MB
    mov word [secs_trk], 18
    mov word [heads], 2
    pop di
    pop es
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; ============================================================
; Lecture CHS fallback avec géométrie détectée
; ============================================================
read_chs_fallback:
    ; Convertir LBA 2 en CHS en utilisant la géométrie détectée
    ; LBA = (cylinder * heads + head) * sectors_per_track + (sector - 1)
    ; Pour LBA 2: cylinder=0, head=0, sector=3 (avec 18 sectors/track, 2 heads)
    mov ax, STAGE2_LBA_START    ; LBA 2
    xor dx, dx
    div word [secs_trk]         ; AX = cylinder * heads + head, DX = sector - 1
    inc dx                      ; DX = sector
    mov cl, dl                  ; CL = sector
    mov ch, 0                   ; CH = cylinder low (0)
    xor dx, dx
    div word [heads]            ; AX = cylinder, DX = head
    mov dh, dl                  ; DH = head
    mov ch, al                  ; CH = cylinder low (0 pour LBA 2)
    
    mov ah, 0x02
    mov al, STAGE2_SECTORS
    mov dl, [boot_drive]
    mov bx, STAGE2_OFF
    mov ax, STAGE2_SEG
    mov es, ax
    int 0x13
    ret

; ============================================================
; DATA
; ============================================================
boot_drive    db 0
heads         dw 2
secs_trk      dw 18
msg_disk      db "Stage1: Disk error", 0x0D, 0x0A, 0
msg_a20       db "Stage1: A20 enable failed", 0x0D, 0x0A, 0

; DAP pour Stage 2 (Disk Address Packet)
align 16
dap:
    db 0x10             ; Taille du paquet (16 octets)
    db 0                ; Réservé
    dw STAGE2_SECTORS   ; Nombre de secteurs à lire
    dw STAGE2_OFF       ; Offset de destination
    dw STAGE2_SEG       ; Segment de destination
    dq STAGE2_LBA_START ; LBA de départ (64-bit)

times 510-($-$$) db 0
dw 0xAA55
