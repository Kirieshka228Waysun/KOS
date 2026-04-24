; ============================================================================
; KOS Bootloader - SERIAL.COM Test Program
; ============================================================================
; 16-bit real mode program that tests serial communication
; Initializes COM1 at 9600 baud and echoes received characters
; ============================================================================

[BITS 16]
[ORG 0x0000]

start:
    ; Setup stack
    mov     ax, 0x2000
    mov     ss, ax
    mov     sp, 0xFFFE
    
    ; Initialize COM1 (9600,8N1)
    mov     dx, 0x3F8 + 1     ; Interrupt Enable Register
    mov     al, 0x00
    out     dx, al            ; Disable interrupts
    
    mov     dx, 0x3F8 + 3     ; Line Control Register
    mov     al, 0x80
    out     dx, al            ; Enable DLAB
    
    mov     dx, 0x3F8 + 0     ; Divisor Latch Low
    mov     al, 0x0C          ; 115200 / 9600 = 12
    out     dx, al
    
    mov     dx, 0x3F8 + 1     ; Divisor Latch High
    mov     al, 0x00
    out     dx, al
    
    mov     dx, 0x3F8 + 3     ; Line Control Register
    mov     al, 0x03          ; 8 bits, no parity, 1 stop
    out     dx, al
    
    mov     dx, 0x3F8 + 2     ; FIFO Control Register
    mov     al, 0x07          ; Enable FIFO, clear buffers
    out     dx, al
    
    mov     dx, 0x3F8 + 4     ; Modem Control Register
    mov     al, 0x0B          ; RTS, DTR, OUT2
    out     dx, al
    
    ; Send "SERIAL OK\r\n" message
    mov     si, msg_serial
    call    print_string
    
    ; Main loop: echo received characters
.echo_loop:
    ; Check if data available
    mov     dx, 0x3F8 + 5     ; Line Status Register
    in      al, dx
    test    al, 0x01          ; Data Ready bit
    jz      .echo_loop
    
    ; Read character
    mov     dx, 0x3F8
    in      al, dx
    
    ; Echo it back
    push    ax
    call    serial_putc
    pop     ax
    
    ; Check for escape key to exit
    cmp     al, 0x1B          ; ESC
    je      .exit
    
    jmp     .echo_loop
    
.exit:
    ; Return to DOS/BIOS
    mov     ah, 0x00
    int     0x20              ; INT 0x20 (PSP return)
    
; ============================================================================
; Print string via serial (SI points to string)
; ============================================================================
print_string:
    pusha
.print_loop:
    lodsb
    test    al, al
    jz      .done
    call    serial_putc
    jmp     .print_loop
.done:
    popa
    ret

; ============================================================================
; Send character via serial (AL = character)
; ============================================================================
serial_putc:
    push    dx
.wait_thre:
    mov     dx, 0x3F8 + 5     ; Line Status Register
    in      al, dx
    test    al, 0x20          ; THRE bit
    jz      .wait_thre
    
    pop     dx
    mov     dx, 0x3F8         ; Transmit Holding Register
    out     dx, al
    ret

; ============================================================================
; Data
; ============================================================================
msg_serial: db "SERIAL OK", 13, 10, 0

; Pad to reasonable size
times 256-($-$$) db 0
