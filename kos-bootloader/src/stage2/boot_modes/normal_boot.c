/* ============================================================================
 * KOS Bootloader - Normal Boot Mode Implementation
 * ============================================================================
 * Loads and executes /boot/boot32.sys kernel from ISO9660
 * Kernel is loaded at 0x10000 (1MB) and executed in protected mode
 * ============================================================================
 */

#include "normal_boot.h"
#include "iso_fs.h"
#include "vga.h"
#include "serial.h"
#include "printf.h"
#include "string.h"
#include "io.h"
#include "boot_params.h"

/* Kernel load address */
#define KERNEL_LOAD_ADDR    0x10000
#define KERNEL_MAX_SIZE     (4 * 1024 * 1024)  /* 4MB max kernel size */

/* Kernel entry point type */
typedef void (*kernel_entry_t)(void);

/**
 * @brief Execute normal boot - load and jump to kernel
 */
void normal_boot(void) {
    iso_file_t* kernel_file;
    int bytes_read;
    
    serial_puts("[BOOT] Starting normal boot sequence...\r\n");
    
    /* Ensure ISO is mounted */
    if (!iso_is_mounted()) {
        if (!iso_mount(0)) {
            vga_print_string(15, 20, "ERROR: Cannot mount ISO9660", 
                           COLOR_RED, COLOR_BLACK);
            serial_puts("[BOOT] ERROR: Cannot mount ISO9660\r\n");
            goto error_halt;
        }
    }
    
    /* Find kernel file */
    vga_print_string(12, 20, "Looking for /boot/boot32.sys...", 
                    COLOR_WHITE, COLOR_BLACK);
    serial_puts("[BOOT] Searching for /boot/boot32.sys...\r\n");
    
    kernel_file = iso_find("/boot/boot32.sys");
    if (kernel_file == NULL) {
        vga_print_string(15, 20, "ERROR: Kernel not found!", 
                        COLOR_RED, COLOR_BLACK);
        serial_puts("[BOOT] ERROR: Kernel /boot/boot32.sys not found\r\n");
        goto error_halt;
    }
    
    /* Check kernel size */
    if (kernel_file->size > KERNEL_MAX_SIZE) {
        vga_printf(15, 20, COLOR_RED, COLOR_BLACK,
                  "ERROR: Kernel too large (%u bytes)", kernel_file->size);
        serial_puts("[BOOT] ERROR: Kernel exceeds maximum size\r\n");
        goto error_halt;
    }
    
    /* Load kernel */
    vga_printf(14, 20, COLOR_WHITE, COLOR_BLACK,
              "Loading kernel (%u bytes)...", kernel_file->size);
    serial_puts("[BOOT] Loading kernel...\r\n");
    
    bytes_read = iso_read(kernel_file, (void*)KERNEL_LOAD_ADDR, 
                         kernel_file->size);
    
    if (bytes_read != (int)kernel_file->size) {
        vga_print_string(15, 20, "ERROR: Failed to load kernel", 
                        COLOR_RED, COLOR_BLACK);
        serial_puts("[BOOT] ERROR: Failed to read kernel file\r\n");
        goto error_halt;
    }
    
    serial_puts("[BOOT] Kernel loaded successfully\r\n");
    serial_puthex(KERNEL_LOAD_ADDR, 8);
    serial_puts("\r\n");
    
    /* Prepare boot parameters */
    boot_params_t* params = get_boot_params();
    boot_params_init(params);
    
    params->kernel_addr = KERNEL_LOAD_ADDR;
    params->kernel_size = kernel_file->size;
    params->kernel_entry = KERNEL_LOAD_ADDR;
    boot_params_set_checksum(params);
    
    /* Display success message */
    vga_clear_screen(COLOR_BLACK);
    vga_print_string_centered(10, "KOS Bootloader", COLOR_WHITE, COLOR_BLUE);
    vga_print_string_centered(12, "Starting kernel...", COLOR_WHITE, COLOR_BLACK);
    vga_printf(15, 30, COLOR_LIGHT_GRAY, COLOR_BLACK,
              "Address: 0x%08X", KERNEL_LOAD_ADDR);
    vga_printf(16, 30, COLOR_LIGHT_GRAY, COLOR_BLACK,
              "Size: %u bytes", kernel_file->size);
    
    serial_puts("[BOOT] Jumping to kernel...\r\n");
    serial_flush();
    
    /* Small delay to allow serial output to complete */
    for (volatile int i = 0; i < 100000; i++);
    
    /* Jump to kernel */
    kernel_entry_t kernel_entry = (kernel_entry_t)KERNEL_LOAD_ADDR;
    
    /* Disable interrupts before jump */
    cli();
    
    /* Jump to kernel - this never returns */
    kernel_entry();
    
    /* Should never reach here */
    return;
    
error_halt:
    /* Display error and halt */
    vga_print_string(20, 20, "Press any key to reboot...", 
                    COLOR_LIGHT_GRAY, COLOR_BLACK);
    
    while (1) {
        hlt();
    }
}
