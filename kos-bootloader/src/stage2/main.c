/* ============================================================================
 * KOS Bootloader - Stage 2 Main Entry Point
 * ============================================================================
 * 32-bit protected mode bootloader with boot menu
 * Loads at 0x7E00, switches to protected mode, initializes drivers
 * ============================================================================
 */

#include "types.h"
#include "io.h"
#include "boot_params.h"
#include "vga.h"
#include "keyboard.h"
#include "serial.h"
#include "iso_fs.h"
#include "printf.h"
#include "string.h"
#include "normal_boot.h"
#include "recovery/console.h"
#include "memtest_stub.h"

/* ============================================================================
 * Memory Layout (Protected Mode)
 * ============================================================================
 * 0x00000000 - 0x0009FFFF: Real mode memory, BIOS areas
 * 0x000A0000 - 0x000BFFFF: VGA framebuffer (Mode 0x12)
 * 0x000C0000 - 0x000FFFFF: ROM areas
 * 0x00100000 - 0x008FFFFF: Kernel and stage2 data (4MB)
 * 0x00900000 - 0x009FFFFF: Stack (1MB)
 * 0x00A00000+: Free memory
 * ============================================================================
 */

#define BOOT_MENU_Y           10
#define BOOT_MENU_X           20
#define BOOT_TITLE_Y          5
#define BOOT_TITLE_X          25

/* Boot mode selection */
typedef enum {
    BOOT_MODE_NORMAL = 0,
    BOOT_MODE_RECOVERY = 1,
    BOOT_MODE_MEMTEST = 2
} boot_mode_t;

/* External symbols from assembly stub */
extern uint32_t gdt_descriptor[2];
extern void load_gdt(uint32_t* gdtr);
extern void switch_to_protected(void);

/* Forward declarations */
static void gdt_setup(void);
static void idt_stub_setup(void);
static void enable_a20(void);
static void bootstrap(void);
static void boot_menu(void);
static boot_mode_t wait_for_selection(void);
static void execute_boot_mode(boot_mode_t mode);

/* GDT Structure (Flat Memory Model) */
#define GDT_ENTRIES 4

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed, aligned(4)));

static struct gdt_entry gdt[GDT_ENTRIES];
static struct gdt_ptr gdtp;

/* GDT selectors */
#define GDT_NULL    0x00
#define GDT_CODE    0x08
#define GDT_DATA    0x10

/* ============================================================================
 * Entry Point (_start) - called from assembly stub
 * ============================================================================
 */
void _start(void) {
    /* Bootstrap the system into protected mode */
    bootstrap();
    
    /* Initialize all drivers */
    serial_init();
    vga_init();
    keyboard_init();
    
    /* Clear screen and show welcome */
    vga_clear_screen(COLOR_BLACK);
    
    serial_puts("\r\n[KOS] Stage 2 loaded successfully\r\n");
    
    /* Show boot menu */
    boot_menu();
}

static void bootstrap(void) {
    /* Setup Global Descriptor Table */
    gdt_setup();
    
    /* Load GDT - use intermediate variable to avoid unaligned pointer warning */
    gdtp.limit = sizeof(gdt) - 1;
    gdtp.base = (uint32_t)(uintptr_t)gdt;
    load_gdt((uint32_t*)(uintptr_t)&gdtp);
    
    /* Enable A20 line (allow access to memory above 1MB) */
    enable_a20();
    
    /* Switch to protected mode */
    switch_to_protected();
    
    /* Setup segment registers after protected mode switch */
    __asm__ volatile(
        "movl $0x10, %%eax\n\t"
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        "movw %%ax, %%fs\n\t"
        "movw %%ax, %%gs\n\t"
        "movw %%ax, %%ss\n\t"
        "movl $0x90000, %%esp\n\t"
        :
        :
        : "eax", "memory"
    );
    
    /* Setup minimal IDT stubs */
    idt_stub_setup();
}

/* ============================================================================
 * Setup Global Descriptor Table
 * Creates flat memory model with code@0x08, data@0x10
 * ============================================================================
 */
static void gdt_setup(void) {
    /* Null descriptor (required) */
    gdt[0].limit_low = 0;
    gdt[0].base_low = 0;
    gdt[0].base_middle = 0;
    gdt[0].access = 0;
    gdt[0].granularity = 0;
    gdt[0].base_high = 0;
    
    /* Code segment: base=0, limit=4GB, DPL=0, present, executable, readable */
    gdt[1].limit_low = 0xFFFF;
    gdt[1].base_low = 0;
    gdt[1].base_middle = 0;
    gdt[1].access = 0x9A;  /* 10011010b: present, DPL=0, code, readable */
    gdt[1].granularity = 0xCF;  /* 4KB granularity, 32-bit, limit in pages */
    gdt[1].base_high = 0;
    
    /* Data segment: base=0, limit=4GB, DPL=0, present, writable */
    gdt[2].limit_low = 0xFFFF;
    gdt[2].base_low = 0;
    gdt[2].base_middle = 0;
    gdt[2].access = 0x92;  /* 10010010b: present, DPL=0, data, writable */
    gdt[2].granularity = 0xCF;
    gdt[2].base_high = 0;
    
    /* Video/data segment (optional, same as data for now) */
    gdt[3].limit_low = 0xFFFF;
    gdt[3].base_low = 0;
    gdt[3].base_middle = 0;
    gdt[3].access = 0x92;
    gdt[3].granularity = 0xCF;
    gdt[3].base_high = 0;
    
    /* Setup GDT pointer */
    gdtp.limit = sizeof(gdt) - 1;
    gdtp.base = (uint32_t)&gdt;
}

/* ============================================================================
 * Minimal IDT Setup (stub handlers for exceptions)
 * ============================================================================
 */
static void idt_stub_setup(void) {
    /* For now, just disable interrupts - full IDT setup comes later */
    asm volatile("cli");
    /* TODO: Implement full IDT with exception handlers */
}

/* ============================================================================
 * Enable A20 Line using Keyboard Controller
 * ============================================================================
 */
static void enable_a20(void) {
    /* Wait for keyboard controller to be ready */
    while (inb(0x64) & 0x02);
    
    /* Send command to write output port */
    outb(0x64, 0xD1);
    
    /* Wait again */
    while (inb(0x64) & 0x02);
    
    /* Enable A20 (set bit 1 of output port) */
    outb(0x60, inb(0x60) | 0x02);
    
    /* Small delay */
    for (volatile int i = 0; i < 1000; i++);
}

/* ============================================================================
 * Display Boot Menu
 * ============================================================================
 */
static void boot_menu(void) {
    const char* title = "KOS Bootloader v0.1-alpha";
    const char* normal_entry = "[G] Normal Boot - Load /boot/boot32.sys";
    const char* recovery_entry = "[R] Recovery Console";
    const char* memtest_entry = "[M] Memory Test";
    
    /* Print title */
    vga_print_string_centered(BOOT_TITLE_Y, title, COLOR_WHITE, COLOR_BLUE);
    
    /* Print menu entries */
    vga_print_string(BOOT_MENU_Y, BOOT_MENU_X, normal_entry, 
                     COLOR_WHITE, COLOR_BLACK);
    vga_print_string(BOOT_MENU_Y + 2, BOOT_MENU_X, recovery_entry,
                     COLOR_WHITE, COLOR_BLACK);
    vga_print_string(BOOT_MENU_Y + 4, BOOT_MENU_X, memtest_entry,
                     COLOR_WHITE, COLOR_BLACK);
    
    /* Highlight default selection */
    vga_print_string(BOOT_MENU_Y, BOOT_MENU_X - 3, ">>", 
                     COLOR_YELLOW, COLOR_BLACK);
    
    /* Wait for user input or timeout */
    boot_mode_t mode = wait_for_selection();
    
    /* Execute selected boot mode */
    execute_boot_mode(mode);
}

/* ============================================================================
 * Wait for User Selection
 * Returns selected boot mode
 * ============================================================================
 */
static boot_mode_t wait_for_selection(void) {
    uint8_t key;
    
    while (1) {
        if (keyboard_check_key()) {
            key = keyboard_get_key();
            
            switch (key) {
                case SCAN_G:
                case SCAN_G | SCAN_RELEASE:
                    return BOOT_MODE_NORMAL;
                    
                case SCAN_R:
                case SCAN_R | SCAN_RELEASE:
                    return BOOT_MODE_RECOVERY;
                    
                case SCAN_M:
                case SCAN_M | SCAN_RELEASE:
                    return BOOT_MODE_MEMTEST;
                    
                case SCAN_UP:
                case SCAN_DOWN:
                    /* Could implement arrow key navigation here */
                    break;
                    
                case SCAN_ENTER:
                    return BOOT_MODE_NORMAL;
                    
                default:
                    break;
            }
        }
        
        /* Small delay to prevent busy-waiting */
        for (volatile int i = 0; i < 100000; i++);
    }
}

/* ============================================================================
 * Execute Selected Boot Mode
 * ============================================================================
 */
static void execute_boot_mode(boot_mode_t mode) {
    vga_clear_screen(COLOR_BLACK);
    
    switch (mode) {
        case BOOT_MODE_NORMAL:
            serial_puts("[KOS] Starting normal boot...\r\n");
            vga_print_string(10, 20, "Loading kernel...", COLOR_WHITE, COLOR_BLACK);
            normal_boot();
            break;
            
        case BOOT_MODE_RECOVERY:
            serial_puts("[KOS] Starting recovery console...\r\n");
            vga_print_string(10, 20, "Starting Recovery Console...", COLOR_WHITE, COLOR_BLACK);
            recovery_console_main();
            break;
            
        case BOOT_MODE_MEMTEST:
            serial_puts("[KOS] Starting memory test...\r\n");
            memtest_main();
            break;
            
        default:
            vga_print_string(15, 20, "Unknown boot mode!", COLOR_RED, COLOR_BLACK);
            break;
    }
}

/* ============================================================================
 * Assembly Stub for Protected Mode Transition
 * This code is embedded in the binary and called during bootstrap
 * ============================================================================
 */
__asm__(
    ".section .stub\n\t"
    ".globl load_gdt\n\t"
    ".globl switch_to_protected\n\t"
    "\n\t"
    "load_gdt:\n\t"
    "    movl 4(%esp), %eax\n\t"      /* Get GDTR address from stack */
    "    lgdt (%eax)\n\t"             /* Load GDT */
    "    ret\n\t"
    "\n\t"
    "switch_to_protected:\n\t"
    "    movl %cr0, %eax\n\t"         /* Get CR0 */
    "    orl $1, %eax\n\t"            /* Set PE bit (bit 0) */
    "    movl %eax, %cr0\n\t"         /* Write back to CR0 */
    "    jmp $0x08, $flush_cs\n\t"    /* Far jump to flush CS */
    "flush_cs:\n\t"
    "    ret\n\t"
    "\n\t"
    ".text\n\t"
);
