; ============================================================================
; KOS Bootloader - ISO9660 Stub Parser (Stage 1)
; ============================================================================
; Minimal ISO9660 parser for finding Stage 2 in real mode
; Supports Primary Volume Descriptor and root directory lookup
; ============================================================================

[BITS 16]

GLOBAL iso9660_init
GLOBAL iso9660_find_file
GLOBAL iso9660_read_file

SECTION .text

; ============================================================================
; Initialize ISO9660 filesystem
; Input: DL = boot drive
; Output: CF=0 success, CF=1 error
; ============================================================================
iso9660_init:
    pusha
    
    ; Read Primary Volume Descriptor at LBA 16
    mov     bx, 0x0000
    mov     es, bx
    mov     bx, iso_buffer      ; Buffer at known location
    mov     al, 1               ; Read 1 sector
    mov     cl, 17              ; Sector 17 (LBA 16)
    mov     ch, 0
    mov     dh, 0
    ; DL already contains boot drive
    call    disk_read_sectors_chs
    jc      .error
    
    ; Verify signature "CD001" at offset 1
    mov     si, iso_buffer + 1
    mov     di, iso_signature_expected
    mov     cx, 5
    repe cmpsb
    jne     .error
    
    ; Extract volume descriptor info
    ; Root directory record at offset 0x9C (156)
    mov     eax, [iso_buffer + 0x9E]    ; LBA of root directory
    mov     [iso_root_lba], eax
    mov     eax, [iso_buffer + 0xA6]    ; Size of root directory
    mov     [iso_root_size], eax
    
    popa
    clc
    ret
.error:
    popa
    stc
    ret

; ============================================================================
; Find file in ISO9660 root directory
; Input: SI = pointer to filename (uppercase, DOS format with extension)
; Output: 
;   CF=0 found, EAX=LBA, EBX=size
;   CF=1 not found
; ============================================================================
iso9660_find_file:
    pusha
    
    ; Load root directory
    mov     bx, 0x0000
    mov     es, bx
    mov     bx, dir_buffer
    mov     al, 8               ; Read up to 8 sectors (16KB root dir)
    mov     ecx, [iso_root_lba]
    
    ; Convert LBA to CHS for reading
    ; Simplified: assume LBA maps directly to sector numbers
    add     cl, 16              ; Add volume descriptor offset
    adc     ch, 0
    mov     dh, 0
    mov     dl, [boot_drive]
    call    disk_read_sectors_chs
    jc      .error
    
    ; Search directory records
    mov     di, dir_buffer
    mov     cx, [iso_root_size]
    xor     bp, bp              ; Record counter
    
.search_loop:
    cmp     cx, 0
    je      .not_found
    
    ; Check record length (first byte)
    mov     al, [di]
    test    al, al
    jz      .not_found          ; Zero-length = end of directory
    
    ; Skip if not a file (bit 2 of file flags at offset 25)
    ; File flags: bit 0=hidden, bit 1=directory, bit 2=associated
    mov     bl, [di + 25]
    test    bl, 2               ; Check if directory
    jnz     .next_record        ; Skip directories
    
    ; Compare filename
    mov     bl, [di + 32]       ; Filename length
    test    bl, bl
    jz      .next_record
    
    ; Quick check: first character
    mov     al, [si]
    mov     ah, [di + 33]
    cmp     al, ah
    jne     .next_record
    
    ; Full comparison (up to 12 chars for DOS name)
    push    si
    push    di
    mov     cx, 12
    mov     bp, di
    mov     dx, si
.compare_loop:
    cmp     cx, 0
    je      .found
    mov     al, [dx]
    cmp     al, 0
    je      .check_short_name
    mov     ah, [bp]
    cmp     al, ah
    jne     .compare_fail
    inc     dx
    inc     bp
    dec     cx
    jmp     .compare_loop
.check_short_name:
    ; Handle short filenames
    mov     ah, [bp]
    cmp     ah, ' '
    je      .found              ; Space padding = end of name
    cmp     ah, '.'
    je      .found
    cmp     ah, ';'
    je      .found
.compare_fail:
    pop     di
    pop     si
    jmp     .next_record
    
.found:
    pop     di
    pop     si
    
    ; Extract file location and size
    mov     eax, [di + 2]       ; LBA of file extent
    mov     ebx, [di + 10]      ; File size
    
    popa
    mov     [file_lba], eax
    mov     [file_size], ebx
    clc
    ret
    
.next_record:
    mov     al, [di]            ; Get record length
    cbw                         ; Sign extend to AX
    add     di, ax
    sub     cx, ax
    jmp     .search_loop
    
.not_found:
    popa
    stc
    ret
    
.error:
    popa
    stc
    ret

; ============================================================================
; Read file from ISO9660
; Input:
;   EAX = file LBA
;   EBX = file size
;   ES:DI = destination buffer
; Output: CF=0 success, CF=1 error
; ============================================================================
iso9660_read_file:
    pusha
    
    ; Calculate sectors to read
    mov     ecx, ebx
    add     ecx, 2047           ; Round up
    shr     ecx, 11             ; Divide by 2048
    mov     [sectors_to_read], cl
    
    ; Read sectors
    mov     al, [sectors_to_read]
    mov     cl, al
    add     cl, 16              ; Offset for LBA
    mov     ch, 0
    mov     dh, 0
    mov     dl, [boot_drive]
    call    disk_read_sectors_chs
    jc      .error
    
    popa
    clc
    ret
.error:
    popa
    stc
    ret

SECTION .data
iso_signature_expected: db "CD001"
boot_drive: db 0

SECTION .bss
iso_buffer: resb 2048           ; ISO9660 sector buffer
dir_buffer: resb 16384          ; Directory buffer (8 sectors)
iso_root_lba: dd 0
iso_root_size: dd 0
file_lba: dd 0
file_size: dd 0
sectors_to_read: db 0
