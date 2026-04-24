/* ============================================================================
 * KOS Bootloader - Linker Script
 * ============================================================================
 * Memory layout for Stage 2 bootloader
 * Loads at 0x7E00, runs in 32-bit protected mode
 * ============================================================================
 */

ENTRY(_start)

MEMORY
{
    /* Stage 2 loads at 0x7E00 (just after boot sector) */
    STAGE2_LMA (rwx) : ORIGIN = 0x00007E00, LENGTH = 32K
    
    /* Code runs from this address */
    STAGE2_VMA (rwx) : ORIGIN = 0x00007E00, LENGTH = 32K
}

SECTIONS
{
    /* Load address and virtual address are the same */
    . = 0x7E00;
    
    .stub : {
        *(.stub)
    } >STAGE2_VMA AT>STAGE2_LMA
    
    .text : {
        _text_start = .;
        *(.text)
        *(.text.*)
        _text_end = .;
    } >STAGE2_VMA AT>STAGE2_LMA
    
    .rodata : {
        _rodata_start = .;
        *(.rodata)
        *(.rodata.*)
        _rodata_end = .;
    } >STAGE2_VMA AT>STAGE2_LMA
    
    .data : {
        _data_start = .;
        *(.data)
        *(.data.*)
        _data_end = .;
    } >STAGE2_VMA AT>STAGE2_LMA
    
    .bss : {
        _bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        _bss_end = .;
    } >STAGE2_VMA AT>STAGE2_LMA
    
    /* Calculate sizes */
    _text_size = _text_end - _text_start;
    _rodata_size = _rodata_end - _rodata_start;
    _data_size = _data_end - _data_start;
    _bss_size = _bss_end - _bss_start;
    
    /* End of used memory */
    _end = .;
    
    /* Discard unwanted sections */
    /DISCARD/ : {
        *(.comment)
        *(.note.*)
        *(.eh_frame)
        *(.eh_frame_hdr)
    }
}
