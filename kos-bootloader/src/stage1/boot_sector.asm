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

; ============================================================================
; Entry Point
; ============================================================================
start:
    ; Disable interrupts during setup
    cli
    
    ; Initialize stack segment and pointer
    xor     ax, ax              ; AX = 0
    mov     ss, ax              ; SS = 0
    mov     sp, 0x7C00          ; SP = 0x7C00 (stack grows downward from entry)
    
    ; Initialize data segments
    mov     ds, ax              ; DS = 0
    mov     es, ax              ; ES = 0
    mov     fs, ax              ; FS = 0
    mov     gs, ax              ; GS = 0
    
    ; Re-enable interrupts
    sti
    
    ; Save boot drive number (from BIOS in DL)
    mov     [boot_drive], dl
    
    ; Print boot message
    mov     si, msg_boot
    call    print_string
    
    ; Initialize disk system
    call    disk_reset
    
    ; Find and load Stage 2 from ISO9660
    call    iso9660_find_stage2
    jc      error_load_stage2
    
    ; Verify stage2 signature (optional debug)
    ; cmp     word [0x7E00], 0x55AA
    ; jne     error_bad_signature
    
    ; Print success message
    mov     si, msg_loading
    call    print_string
    
    ; Jump to Stage 2 at 0x7E00
    jmp     0x0000:0x7E00

; ============================================================================
; Error Handling
; ============================================================================
error_load_stage2:
    mov     si, msg_error_stage2
    call    print_string
    mov     al, 0x01            ; Error code 1: Stage2 not found
    jmp     error_halt

error_bad_signature:
    mov     si, msg_error_sig
    call    print_string
    mov     al, 0x02            ; Error code 2: Bad signature
    jmp     error_halt

error_halt:
    ; Flash cursor and beep
    call    pc_speaker_beep
.halt_loop:
    hlt
    jmp     .halt_loop

; ============================================================================
; Print String (SI points to null-terminated string)
; Uses BIOS interrupt 0x10, function 0x0E (teletype output)
; ============================================================================
print_string:
    pusha
    mov     ah, 0x0E            ; Teletype output function
.print_loop:
    lodsb                       ; Load byte from SI into AL, increment SI
    test    al, al              ; Check if null terminator
    jz      .done
    int     0x10                ; BIOS video interrupt
    jmp     .print_loop
.done:
    popa
    ret

; ============================================================================
; Disk Reset (reset disk system)
; ============================================================================
disk_reset:
    pusha
    mov     dl, [boot_drive]    ; Get boot drive
    mov     ah, 0x00            ; Reset disk function
    int     0x13                ; BIOS disk interrupt
    jc      .error
    popa
    ret
.error:
    popa
    stc                         ; Set carry flag on error
    ret

; ============================================================================
; Read Sectors (CHS mode)
; Input: 
;   AL = number of sectors to read
;   CH = cylinder (0-1023, bits 0-7 in CH, bits 8-9 in CL bits 6-7)
;   CL = sector (bits 0-5), cylinder high bits (bits 6-7)
;   DH = head (0-255)
;   DL = drive number
;   ES:BX = buffer address
; Output: CF set on error
; ============================================================================
disk_read_sectors:
    pusha
.read_retry:
    mov     ah, 0x02            ; Read sectors function
    int     0x13                ; BIOS disk interrupt
    jc      .error
    popa
    clc                         ; Clear carry flag (success)
    ret
.error:
    ; Try resetting and retrying
    call    disk_reset
    popa
    stc                         ; Set carry flag (error)
    ret

; ============================================================================
; ISO9660 Find Stage 2
; Searches for STAGE2.SYS in the root directory of ISO9660 filesystem
; ============================================================================
iso9660_find_stage2:
    pusha
    
    ; Volume Descriptor starts at sector 16 (LBA)
    mov     [iso_lba_base], dword 16
    
    ; Read Volume Descriptor (sector 16)
    mov     bx, 0x0000          ; ES:BX = 0x0000:0x7E00 (reuse stage2 area temporarily)
    mov     es, bx
    mov     bx, 0x7E00
    mov     al, 1               ; Read 1 sector
    mov     cl, 17              ; Sector 17 (LBA 16, CHS: C=0, H=0, S=17)
    mov     ch, 0
    mov     dh, 0
    mov     dl, [boot_drive]
    call    disk_read_sectors
    jc      .error
    
    ; Verify ISO9660 signature "CD001" at offset 1
    mov     si, 0x7E01
    mov     di, iso_signature
    mov     cx, 5
    repe cmpsb
    jne     .error
    
    ; Get root directory record location from Volume Descriptor
    ; Root directory record starts at offset 0x9C (156) for Primary VD
    mov     esi, 0x7E9C         ; Point to root directory record
    
    ; Extract LBA of root directory (little-endian at offset 2)
    mov     eax, [esi + 2]      ; LBA of root directory extent
    mov     [root_dir_lba], eax
    
    ; Extract size of root directory (little-endian at offset 10)
    mov     eax, [esi + 10]     ; Data length of root directory
    mov     [root_dir_size], eax
    
    ; Calculate number of sectors to read for root directory
    add     eax, 2047           ; Round up
    shr     eax, 11             ; Divide by 2048 (sector size)
    mov     [root_sectors], al
    
    ; Read root directory
    mov     bx, 0x0000
    mov     es, bx
    mov     bx, 0x8000          ; Buffer at 0x8000
    mov     al, [root_sectors]
    
    ; Convert LBA to CHS (simplified for first tracks)
    ; LBA = (C * heads + H) * sectors_per_track + (S - 1)
    ; For simplicity, assume CHS: C=0, H=0, S=LBA+17
    mov     ecx, [root_dir_lba]
    add     ecx, 16             ; Add volume descriptor offset
    mov     cl, ch              ; Low byte of cylinder
    add     cl, 17              ; First data sector is 17
    mov     ch, 0
    mov     dh, 0
    mov     dl, [boot_drive]
    call    disk_read_sectors
    jc      .error
    
    ; Search for STAGE2.SYS in root directory
    mov     di, 0x8000          ; Start of root directory buffer
    mov     cx, [root_dir_size] ; Size of root directory
.search_loop:
    cmp     cx, 0
    je      .not_found
    
    ; Check directory record length (first byte)
    mov     al, [di]
    test    al, al
    jz      .next_record        ; Skip zero-length records
    
    ; Check file identifier length (byte 32)
    mov     bl, [di + 32]       ; File identifier length
    test    bl, bl
    jz      .next_record
    
    ; Compare filename "STAGE2.SYS"
    mov     si, stage2_filename
    mov     bp, 12              ; Length of "STAGE2.SYS;1"
    mov     bh, 0
.compare_name:
    cmp     bh, bp
    jge     .found_match
    mov     al, [si + bh]
    mov     ah, [di + 33 + bh]
    ; Convert to uppercase for comparison
    cmp     al, 'a'
    jb      .no_upper_al
    cmp     al, 'z'
    ja      .no_upper_al
    sub     al, 20h             ; Convert to uppercase
.no_upper_al:
    cmp     ah, 'a'
    jb      .no_upper_ah
    cmp     ah, 'z'
    ja      .no_upper_ah
    sub     ah, 20h             ; Convert to uppercase
.no_upper_ah:
    cmp     al, ah
    jne     .next_record
    inc     bh
    jmp     .compare_name
    
.found_match:
    ; Found STAGE2.SYS - extract LBA and load it
    mov     eax, [di + 2]       ; LBA of file extent
    mov     [stage2_lba], eax
    mov     eax, [di + 10]      ; File size
    mov     [stage2_size], eax
    
    ; Calculate sectors to load
    add     eax, 2047
    shr     eax, 11
    mov     [stage2_sectors], al
    
    ; Load stage2.sys to 0x7E00
    mov     bx, 0x0000
    mov     es, bx
    mov     bx, 0x7E00          ; Load address
    mov     al, [stage2_sectors]
    
    ; Convert LBA to CHS and read
    mov     ecx, [stage2_lba]
    ; Simple LBA to CHS conversion (assumes small disk)
    ; This is a simplification - production code needs proper LBA48
    mov     cl, ch
    add     cl, 17
    mov     ch, 0
    mov     dh, 0
    mov     dl, [boot_drive]
    call    disk_read_sectors
    jc      .error
    
    popa
    clc
    ret
    
.next_record:
    mov     al, [di]            ; Get record length
    add     di, ax              ; Move to next record
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
; PC Speaker Beep (error indication)
; ============================================================================
pc_speaker_beep:
    pusha
    in      al, 0x61            ; Get keyboard controller port A
    or      al, 3               ; Set speaker bits
    out     0x61, al
    
    ; Generate tone using PIT channel 2
    mov     al, 0xB6            ; Command for channel 2, square wave
    out     0x43, al
    mov     al, 0x07            ; Low byte of divisor (1000 Hz)
    out     0x42, al
    mov     al, 0x04            ; High byte of divisor
    out     0x42, al
    
    ; Wait briefly
    mov     cx, 0xFFFF
.delay_loop:
    loop    .delay_loop
    
    ; Turn off speaker
    in      al, 0x61
    and     al, 0xFC
    out     0x61, al
    popa
    ret

; ============================================================================
; Data Section
; ============================================================================
msg_boot:             db "KOS Boot v0.1", 13, 10, 0
msg_loading:          db "Loading Stage2...", 13, 10, 0
msg_error_stage2:     db "ERROR: Stage2 not found!", 13, 10, 0
msg_error_sig:        db "ERROR: Bad Stage2 signature!", 13, 10, 0
stage2_filename:      db "STAGE2.SYS;1"
iso_signature:        db "CD001"

boot_drive:           db 0
iso_lba_base:         dd 0
root_dir_lba:         dd 0
root_dir_size:        dd 0
root_sectors:         db 0
stage2_lba:           dd 0
stage2_size:          dd 0
stage2_sectors:       db 0

; ============================================================================
; Padding and Boot Signature
; ============================================================================
times 510-($-$$) db 0   ; Pad to exactly 510 bytes
dw 0xAA55               ; Boot signature (little-endian: 0x55AA)
