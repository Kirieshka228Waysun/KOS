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
    call    toupper
    xchg    al, ah
    call    toupper
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
    mov     di, 0x7A00
.fsys:
    test    byte [di], 0
    jz      err
    mov     cl, [di + 32]
    test    cl, cl
    jz      err
    
    ; Compare "STAGE2.SYS" (12 chars)
    mov     si, stage2_name
    mov     cx, 12
.cname:
    lodsb
    mov     ah, [di + 33]
    call    toupper
    xchg    al, ah
    call    toupper
    cmp     al, ah
    jne     .nsys
    inc     di
    loop    .cname
    sub     di, 12
    jmp     .found
    
.nsys:
    sub     di, 11
    mov     cl, [di + 33]
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

; Uppercase AL if lowercase
toupper:
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
