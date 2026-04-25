; ============================================================================
; KOS Bootloader - Stage 1 (Boot Sector)
; ============================================================================
; MBR/VBR bootloader that loads Stage 2 from ISO9660 filesystem
; Size: exactly 512 bytes with 0xAA55 signature
; Load address: 0x7C00
; Target: BIOS real mode, i686
; ============================================================================

[BITS 16]
[ORG 0x7C00]

start:
    cli
    xor     ax, ax
    mov     ss, ax
    mov     sp, 0x7C00
    mov     ds, ax
    mov     es, ax
    sti
    mov     [boot_drive], dl
    
    ; Reset disk
    mov     dl, [boot_drive]
    mov     ah, 0x00
    int     0x13
    jc      err
    
    ; Read Primary Volume Descriptor at LBA 16
    mov     eax, 16
    mov     bx, 0x7E00
    call    read_lba
    jc      err
    
    ; Verify ISO9660 signature "CD001" at offset 1
    mov     si, 0x7E01
    mov     di, iso_sig
    mov     cx, 5
    repe cmpsb
    jne     err
    
    ; Get Root Directory Record from PVD (offset 156)
    mov     esi, 0x7E00 + 156
    mov     eax, [esi + 2]          ; Root dir LBA
    mov     [root_lba], eax
    
    ; Load root directory (8 sectors)
    mov     ebx, 0x7C00
    mov     cl, 8
.rloop:
    push    eax
    push    ebx
    push    ecx
    call    read_lba
    pop     ecx
    pop     ebx
    pop     eax
    inc     eax
    add     ebx, 512
    dec     cl
    jnz     .rloop
    
    ; Search for BOOT directory in root
    mov     di, 0x7C00
.fboot:
    test    byte [di], 0
    jz      err
    mov     cl, [di + 32]
    test    cl, cl
    jz      err
    
    ; Check for "BOOT" (4 chars)
    mov     si, boot_name
    mov     cx, 4
.cboot:
    lodsb
    mov     ah, [di + 33]
    call    toupper_char
    xchg    al, ah
    call    toupper_char
    cmp     al, ah
    jne     .nrec
    inc     di
    loop    .cboot
    sub     di, 4
    jmp     .fboot_ok
.nrec:
    sub     di, 3
    mov     cl, [di + 35]
    add     di, cx
    cmp     di, 0x7C00 + 4096
    jae     err
    jmp     .fboot
    
.fboot_ok:
    ; Found BOOT dir - get LBA
    mov     eax, [di + 2]
    mov     [boot_lba], eax
    
    ; Load BOOT directory to 0x7A00
    mov     ebx, 0x7A00
    mov     cl, 8
.bloop:
    push    eax
    push    ebx
    push    ecx
    call    read_lba
    pop     ecx
    pop     ebx
    pop     eax
    inc     eax
    add     ebx, 512
    dec     cl
    jnz     .bloop
    
    ; Search for STAGE2.SYS in BOOT dir
    ; ISO9660 format: filename is at offset 33, length at offset 32
    ; Name format: "STAGE2  SYS" or "STAGE2.SYS;1"
    mov     di, 0x7A00
.fsys:
    test    byte [di], 0
    jz      err
    mov     cl, [di + 32]           ; File identifier length
    test    cl, cl
    jz      .nsys                   ; Skip if length is 0
    
    ; Check if this is a directory record (file flags at offset 25)
    test    byte [di + 25], 2       ; Bit 1 = directory
    jnz     .nsys                   ; Skip directories
    
    ; Compare with "STAGE2.SYS" (ignore version suffix ";1")
    ; We need to match: STAGE2 followed by SYS (with space or dot separator)
    mov     si, stage2_name
    mov     ch, 0                   ; Match counter
    mov     bl, cl                  ; Save original length
.match_loop:
    cmp     ch, 12                  ; Max 12 chars to check
    jge     .check_done
    cmp     ch, bl
    jge     .check_done
    
    lodsb                           ; AL = expected char
    mov     si, di
    add     si, 33
    add     si, cx                  ; SI = DI + 33 + CH (using CX since CH is part of it)
    mov     ah, [si]                ; AH = actual char from record
    
    ; Handle case conversion and special chars
    push    ax
    call    toupper_char
    mov     al, ah
    pop     ax
    call    toupper_char
    xchg    al, ah
    
    ; Special handling: space can match dot
    cmp     al, '.'
    jne     .no_dot_space
    cmp     ah, ' '
    je      .match_ok
.no_dot_space:
    cmp     al, ' '
    jne     .no_space_dot
    cmp     ah, '.'
    je      .match_ok
.no_space_dot:
    cmp     al, ah
    jne     .nsys
.match_ok:
    inc     ch
    jmp     .match_loop
    
.check_done:
    ; Also verify no extra significant chars (ignore version ";1")
    jmp     .found
    
.nsys:
    sub     di, 33
    mov     cl, [di + 32]           ; Get record length from offset 32
    test    cl, cl
    jz      err                     ; Safety check
    add     di, cx
    cmp     di, 0x7A00 + 4096
    jae     err
    jmp     .fsys
    
.found:
    ; Load STAGE2.SYS to 0x7E00
    mov     eax, [di + 2]           ; LBA
    mov     ebx, 0x7E00
    mov     cl, 8                   ; Sectors to load
.lsys:
    push    eax
    push    ebx
    push    ecx
    call    read_lba
    pop     ecx
    pop     ebx
    pop     eax
    inc     eax
    add     ebx, 512
    dec     cl
    jnz     .lsys
    
    jmp     0x0000:0x7E00

err:
    cli
    hlt

; Read 1 sector: EAX=LBA, BX=buffer
read_lba:
    pushad
    push    eax
    xor     edx, edx
    mov     ecx, 32
    div     ecx
    inc     dl
    mov     cl, dl
    xor     edx, edx
    mov     ecx, 64
    div     ecx
    mov     dh, dl
    mov     ch, al
    pop     eax
    mov     ax, 0x0201
    mov     dl, [boot_drive]
    int     0x13
    popad
    ret

; Uppercase AL if lowercase (preserves other registers except flags)
toupper_char:
    cmp     al, 'a'
    jb      .ok
    cmp     al, 'z'
    ja      .ok
    sub     al, 20h
.ok:
    ret

; Data
boot_drive:     db 0
iso_sig:        db "CD001"
boot_name:      db "BOOT"
stage2_name:    db "STAGE2.SYS"
root_lba:       dd 0
boot_lba:       dd 0

times 510-($-$$) db 0
dw 0xAA55
