; ============================================================================
; KOS Bootloader - Disk I/O Routines
; ============================================================================
; Low-level disk access using BIOS INT 0x13
; Supports CHS and LBA modes
; ============================================================================

[BITS 16]

GLOBAL disk_reset
GLOBAL disk_read_sectors_chs
GLOBAL disk_read_sectors_lba
GLOBAL disk_write_sectors
GLOBAL lba_to_chs

SECTION .text

; ============================================================================
; Reset Disk System
; ============================================================================
disk_reset:
    pusha
    mov     dl, [boot_drive]    ; Get boot drive number
    mov     ah, 0x00            ; Reset function
    int     0x13                ; BIOS disk interrupt
    jc      .error
    popa
    clc                         ; Clear carry (success)
    ret
.error:
    popa
    stc                         ; Set carry (error)
    ret

; ============================================================================
; Read Sectors - CHS Mode
; Input:
;   AL = sectors to read (1-128)
;   CH = cylinder low bits (0-255)
;   CL = sector (bits 0-5), cylinder high bits (bits 6-7)
;   DH = head (0-255)
;   DL = drive number
;   ES:BX = buffer address
; Output: CF=0 success, CF=1 error
; ============================================================================
disk_read_sectors_chs:
    pusha
.read_attempt:
    push    bx                  ; Save buffer offset
    mov     ah, 0x02            ; Read sectors function
    int     0x13
    pop     bx
    jc      .read_error
    popa
    clc
    ret
.read_error:
    ; Retry with reset
    call    disk_reset
    popa
    stc
    ret

; ============================================================================
; Read Sectors - LBA Mode (INT 0x13 Extended)
; Input:
;   ESI = pointer to Disk Address Packet (DAP)
;   DL = drive number
; Output: CF=0 success, CF=1 error
; 
; DAP structure (16 bytes):
;   [0]  = packet size (16)
;   [1]  = reserved (0)
;   [2-3]= number of blocks
;   [4-5]= transfer buffer offset
;   [6-7]= transfer buffer segment
;   [8-15]= starting absolute block (LBA)
; ============================================================================
disk_read_sectors_lba:
    pusha
.read_ext_attempt:
    mov     ah, 0x42            ; Extended read function
    int     0x13
    jc      .read_ext_error
    popa
    clc
    ret
.read_ext_error:
    call    disk_reset
    popa
    stc
    ret

; ============================================================================
; Write Sectors - LBA Mode
; Input:
;   ESI = pointer to Disk Address Packet
;   DL = drive number
; Output: CF=0 success, CF=1 error
; ============================================================================
disk_write_sectors:
    pusha
.write_ext_attempt:
    mov     ah, 0x43            ; Extended write function
    int     0x13
    jc      .write_ext_error
    popa
    clc
    ret
.write_ext_error:
    call    disk_reset
    popa
    stc
    ret

; ============================================================================
; Convert LBA to CHS
; Input:
;   EAX = LBA address
;   EBX = sectors per track
;   ECX = heads per cylinder
; Output:
;   CH = cylinder low 8 bits
;   CL = sector (bits 0-5), cylinder high 2 bits (bits 6-7)
;   DH = head
;   DL = drive number (preserved)
; Clobbers: EAX, EBX, ECX
; ============================================================================
lba_to_chs:
    push    edx                 ; Save drive number
    push    edi
    
    ; Sector = (LBA % sectors_per_track) + 1
    xor     edx, edx
    div     ebx                 ; EAX = LBA / SPT, EDX = LBA % SPT
    inc     edx
    mov     cl, dl              ; CL = sector
    mov     eax, [esp + 12]     ; Restore quotient (LBA / SPT)
    
    ; Head = (LBA / sectors_per_track) % heads
    xor     edx, edx
    div     ecx                 ; EAX = (LBA/SPT) / HPC, EDX = (LBA/SPT) % HPC
    mov     dh, dl              ; DH = head
    
    ; Cylinder = (LBA / sectors_per_track) / heads
    mov     ch, al              ; CH = cylinder low byte
    shl     ah, 6
    or      cl, ah              ; CL bits 6-7 = cylinder high bits
    
    pop     edi
    pop     edx                 ; Restore drive number
    ret

SECTION .data
boot_drive: db 0

SECTION .bss
dap_buffer: resb 16             ; Disk Address Packet buffer
