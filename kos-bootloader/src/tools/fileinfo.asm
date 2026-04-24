; ============================================================================
; KOS Bootloader - FILEINFO.COM Utility
; ============================================================================
; 16-bit real mode program that reads MBR and displays signature
; For use in recovery console
; ============================================================================

[BITS 16]
[ORG 0x0000]

start:
    ; Setup stack
    mov     ax, 0x2000
    mov     ss, ax
    mov     sp, 0xFFFE
    
    ; Print header
    mov     si, header_msg
    call    print_string
    
    ; Read first sector (MBR) from boot drive
    mov     ax, 0x0000
    mov     es, ax
    mov     bx, 0x7C00          ; Read to same location as boot sector
    
    mov     ah, 0x02            ; Read sectors
    mov     al, 0x01            ; Read 1 sector
    mov     ch, 0x00            ; Cylinder 0
    mov     cl, 0x01            ; Sector 1
    mov     dh, 0x00            ; Head 0
    mov     dl, 0x80            ; Drive 0x80 (first hard disk)
    
    int     0x13                ; BIOS disk interrupt
    jc      .read_error
    
    ; Check MBR signature
    mov     ax, [es:bx + 510]   ; Signature at offset 510
    cmp     ax, 0xAA55
    jne     .bad_signature
    
    ; Success - print OK message
    mov     si, ok_msg
    call    print_string
    
    ; Print signature value
    mov     si, sig_msg
    call    print_string
    
    ; Convert AX to hex and print
    push    ax
    call    print_hex_word
    pop     ax
    
    mov     si, newline
    call    print_string
    
    ; Print drive info
    mov     si, drive_msg
    call    print_string
    
    jmp     .exit
    
.read_error:
    mov     si, error_read_msg
    call    print_string
    jmp     .exit
    
.bad_signature:
    mov     si, error_sig_msg
    call    print_string
    mov     si, found_msg
    call    print_string
    push    ax
    call    print_hex_word
    pop     ax
    
.exit:
    ; Return to caller
    int     0x20

; ============================================================================
; Print string (SI points to null-terminated string)
; ============================================================================
print_string:
    pusha
.loop:
    lodsb
    test    al, al
    jz      .done
    mov     ah, 0x0E
    int     0x10
    jmp     .loop
.done:
    popa
    ret

; ============================================================================
; Print 16-bit hex value (AX)
; ============================================================================
print_hex_word:
    push    ax
    push    cx
    push    dx
    
    mov     cx, 4             ; 4 nibbles
    rol     ax, 4             ; Rotate to get high nibble first
    
.print_nibble:
    push    cx
    push    ax
    and     al, 0x0F          ; Get low nibble
    cmp     al, 10
    jb      .digit
    add     al, 'A' - 10 - '0'
.digit:
    add     al, '0'
    mov     ah, 0x0E
    int     0x10
    pop     ax
    pop     cx
    rol     ax, 4
    loop    .print_nibble
    
    pop     dx
    pop     cx
    pop     ax
    ret

; ============================================================================
; Data
; ============================================================================
header_msg:     db 13, 10, "FILEINFO Utility v0.1", 13, 10
                db "Reading MBR...", 13, 10, 0
ok_msg:         db 13, 10, "MBR Signature: 0x", 0
sig_msg:        db " [OK]", 13, 10, 0
error_read_msg: db 13, 10, "ERROR: Failed to read MBR!", 13, 10, 0
error_sig_msg:  db 13, 10, "ERROR: Invalid MBR signature!", 13, 10, 0
found_msg:      db "Found: 0x", 0
drive_msg:      db 13, 10, "Drive: 0x80 (First Hard Disk)", 13, 10, 0
newline:        db 13, 10, 0

; No padding needed - COM files can be any size < 64KB
; The program will return via INT 0x20 before reaching here
end_marker:
