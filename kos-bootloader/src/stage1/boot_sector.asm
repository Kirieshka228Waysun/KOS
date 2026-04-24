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
    jc      disk_error
    
    ; Read Volume Descriptor at LBA 16
    mov     eax, 16
    mov     bx, 0x7E00
    call    read_lba
    jc      disk_error
    
    ; Verify ISO9660 signature "CD001"
    mov     si, 0x7E01
    mov     di, iso_signature
    mov     cx, 5
    repe cmpsb
    jne     not_iso
    
    ; Read root directory at LBA 20
    mov     eax, 20
    mov     bx, 0x7C00
    call    read_lba
    jc      disk_error
    
    ; Search for STAGE2.SYS in root directory
    mov     di, 0x7C00
.find_stage2:
    mov     bl, [di]
    test    bl, bl
    jz      stage2_not_found
    
    ; Compare filename at offset 33
    mov     si, stage2_name
    mov     cx, 12
.check_name:
    lodsb
    mov     ah, [di + 33]
    inc     di
    ; Uppercase compare
    cmp     al, 'a'
    jb      .skip_upper_al
    cmp     al, 'z'
    ja      .skip_upper_al
    sub     al, 20h
.skip_upper_al:
    cmp     ah, 'a'
    jb      .skip_upper_ah
    cmp     ah, 'z'
    ja      .skip_upper_ah
    sub     ah, 20h
.skip_upper_ah:
    cmp     al, ah
    jne     .next_record
    loop    .check_name
    
    ; Found! Get LBA from offset 2
    sub     di, 12
    mov     eax, [di + 2]
    mov     ebx, 0x7E00
    call    read_lba
    jc      disk_error
    
    jmp     0x0000:0x7E00
    
.next_record:
    mov     cl, [di + 32]
    add     di, cx
    jmp     .find_stage2

disk_error:
not_iso:
stage2_not_found:
    cli
    hlt

; Read LBA sector
read_lba:
    pushad
    push    eax
    ; Convert LBA to CHS
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
    mov     bx, 0x7E00
    mov     dl, [boot_drive]
    int     0x13
    popad
    ret

; Data
boot_drive:         db 0
iso_signature:      db "CD001"
stage2_name:        db "STAGE2.SYS"
name_len:           equ $ - stage2_name

times 510-($-$$) db 0
dw 0xAA55
