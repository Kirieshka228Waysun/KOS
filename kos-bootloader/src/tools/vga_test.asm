; ============================================================================
; KOS Bootloader - VGA.COM Test Program
; ============================================================================
; 16-bit real mode program that tests VGA mode 0x12 (640x480x16)
; Draws a test pattern and waits for keypress
; ============================================================================

[BITS 16]
[ORG 0x0000]

start:
    ; Setup stack
    mov     ax, 0x2000
    mov     ss, ax
    mov     sp, 0xFFFE
    
    ; Set VGA mode 0x12 (640x480x16)
    mov     ax, 0x0012
    int     0x10
    
    ; Clear screen to black
    mov     ax, 0x0600
    mov     bh, 0x00          ; Black
    mov     cx, 0x0000        ; Upper left
    mov     dx, 0x18EF        ; Lower right (24*30-1 in text coords)
    int     0x10
    
    ; Draw border rectangle
    mov     si, border_msg
    call    print_string_vga
    
    ; Draw "VGA TEST @ 0x12" message
    mov     si, vga_msg
    call    print_string_vga
    
    ; Draw colored boxes
    call    draw_color_boxes
    
    ; Wait for keypress
    mov     ah, 0x00
    int     0x16              ; BIOS keyboard read
    
    ; Return to caller
    int     0x20              ; INT 0x20 return

; ============================================================================
; Print string using BIOS teletype (SI points to string)
; ============================================================================
print_string_vga:
    pusha
.print_loop:
    lodsb
    test    al, al
    jz      .done
    mov     ah, 0x0E          ; Teletype output
    mov     bh, 0x00          ; Page 0
    int     0x10
    jmp     .print_loop
.done:
    popa
    ret

; ============================================================================
; Draw colored boxes as test pattern
; ============================================================================
draw_color_boxes:
    pusha
    
    ; For each color (1-15), draw a small box
    mov     bx, 1             ; Start with color 1 (blue)
    mov     dl, 5             ; Starting row
    mov     dh, 10            ; Starting column
    
.draw_loop:
    cmp     bx, 16
    jge     .done_boxes
    
    ; Set cursor position
    mov     ah, 0x02
    mov     bh, 0x00
    mov     dh, dl            ; Row
    mov     dl, dh            ; Column (oops, need to fix)
    int     0x10
    
    ; Print colored block
    mov     ah, 0x09          ; Write attribute/character
    mov     al, 0xDB          ; Solid block character
    mov     bh, 0x00
    mov     bl, cl            ; Color attribute (use CL)
    mov     cx, 5             ; Repeat 5 times
    int     0x10
    
    inc     cl
    inc     dh                ; Move to next column
    jmp     .draw_loop
    
.done_boxes:
    popa
    ret

; ============================================================================
; Data
; ============================================================================
border_msg:     db 13, 10, "================================", 13, 10, 0
vga_msg:        db "   VGA TEST @ MODE 0x12   ", 13, 10
                db "   640x480x16 colors    ", 13, 10
                db "=============================", 13, 10
                db "Press any key to exit...", 13, 10, 0

; Pad to reasonable size
times 512-($-$$) db 0
