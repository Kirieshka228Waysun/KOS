/* ============================================================================
 * KOS Bootloader - Recovery Console Implementation
 * ============================================================================
 * Interactive command-line interface for system recovery
 * Supports: ver, info, exec, help, reboot commands
 * ============================================================================
 */

#include "console.h"
#include "vga.h"
#include "serial.h"
#include "keyboard.h"
#include "printf.h"
#include "string.h"
#include "io.h"
#include "iso_fs.h"

/* Console configuration */
#define CONSOLE_INPUT_MAX   128
#define CONSOLE_HISTORY_SIZE 3
#define CONSOLE_PROMPT      "KOS> "

/* Console state */
static char input_buffer[CONSOLE_INPUT_MAX];
static size_t input_pos = 0;
static char history[CONSOLE_HISTORY_SIZE][CONSOLE_INPUT_MAX];
static int history_index = -1;
static int history_pos = 0;

/* Forward declarations */
static void console_init(void);
static void console_display_banner(void);
static void console_print_prompt(void);
static bool console_read_line(char* buffer, size_t max_len);
static void console_process_command(const char* cmd);
static void console_add_history(const char* cmd);
static void console_execute_cmd(const char* cmd);

/* Command handlers */
static void cmd_ver(const char* args);
static void cmd_info(const char* args);
static void cmd_help(const char* args);
static void cmd_exec(const char* args);
static void cmd_reboot(const char* args);
static void cmd_mem(const char* args);

/* Command table */
typedef struct {
    const char* name;
    const char* description;
    void (*handler)(const char* args);
} command_t;

static const command_t commands[] = {
    {"ver",    "Show version information",           cmd_ver},
    {"info",   "Show system information",            cmd_info},
    {"help",   "Show this help message",             cmd_help},
    {"exec",   "Execute .COM file: exec <file>",     cmd_exec},
    {"mem",    "Show memory map",                    cmd_mem},
    {"reboot", "Reboot the system",                  cmd_reboot},
    {NULL,     NULL,                                 NULL}
};

/**
 * @brief Initialize console state
 */
static void console_init(void) {
    memset(input_buffer, 0, sizeof(input_buffer));
    memset(history, 0, sizeof(history));
    input_pos = 0;
    history_index = -1;
    history_pos = 0;
}

/**
 * @brief Display welcome banner
 */
static void console_display_banner(void) {
    vga_clear_screen(COLOR_BLACK);
    
    vga_print_string_centered(2, "KOS Recovery Console", COLOR_WHITE, COLOR_BLUE);
    vga_print_string_centered(3, "v0.1-alpha", COLOR_LIGHT_GRAY, COLOR_BLUE);
    
    vga_print_string(6, 5, "Type 'help' for available commands", 
                    COLOR_LIGHT_GRAY, COLOR_BLACK);
    vga_print_string(7, 5, "Press Ctrl+C to exit", 
                    COLOR_LIGHT_GRAY, COLOR_BLACK);
    
    serial_puts("\r\n========================================\r\n");
    serial_puts("KOS Recovery Console v0.1-alpha\r\n");
    serial_puts("Type 'help' for commands\r\n");
    serial_puts("========================================\r\n\r\n");
}

/**
 * @brief Print command prompt
 */
static void console_print_prompt(void) {
    static int prompt_row = 10;
    
    vga_print_string(prompt_row, 5, CONSOLE_PROMPT, COLOR_GREEN, COLOR_BLACK);
    serial_puts(CONSOLE_PROMPT);
}

/**
 * @brief Read a line of input from keyboard
 */
static bool console_read_line(char* buffer, size_t max_len) {
    size_t pos = 0;
    int cursor_row = vga_get_cursor_row();
    
    memset(buffer, 0, max_len);
    
    while (1) {
        if (keyboard_check_key()) {
            uint8_t key = keyboard_get_key();
            uint8_t scan = key & 0x7F;
            bool released = (key & SCAN_RELEASE) != 0;
            
            if (released) {
                continue;
            }
            
            switch (scan) {
                case SCAN_ENTER:
                    /* Return the line */
                    serial_puts("\r\n");
                    return true;
                    
                case SCAN_BACKSPACE:
                    if (pos > 0) {
                        pos--;
                        buffer[pos] = '\0';
                        
                        /* Erase character on screen */
                        vga_set_cursor(cursor_row, 5 + strlen(CONSOLE_PROMPT) + pos);
                        vga_draw_char(cursor_row, 5 + strlen(CONSOLE_PROMPT) + pos, 
                                     ' ', COLOR_WHITE, COLOR_BLACK);
                        vga_set_cursor(cursor_row, 5 + strlen(CONSOLE_PROMPT) + pos);
                        
                        serial_puts("\b \b");
                    }
                    break;
                    
                case SCAN_ESCAPE:
                    /* Clear input */
                    while (pos > 0) {
                        pos--;
                        serial_puts("\b \b");
                    }
                    buffer[0] = '\0';
                    vga_set_cursor(cursor_row, 5 + strlen(CONSOLE_PROMPT));
                    return false;
                    
                default:
                    /* Regular character */
                    if (pos < max_len - 1 && scan < 128) {
                        char c = scan_ascii_table[scan];
                        if (c != 0) {
                            buffer[pos++] = c;
                            buffer[pos] = '\0';
                            
                            /* Echo to screen */
                            vga_draw_char(cursor_row, 5 + strlen(CONSOLE_PROMPT) + pos - 1,
                                         c, COLOR_WHITE, COLOR_BLACK);
                            serial_putc(c);
                        }
                    }
                    break;
            }
        }
        
        /* Small delay */
        for (volatile int i = 0; i < 1000; i++);
    }
}

/**
 * @brief Add command to history
 */
static void console_add_history(const char* cmd) {
    if (cmd == NULL || cmd[0] == '\0') {
        return;
    }
    
    /* Shift history */
    for (int i = CONSOLE_HISTORY_SIZE - 1; i > 0; i--) {
        strcpy(history[i], history[i-1]);
    }
    
    /* Add new command */
    strncpy(history[0], cmd, CONSOLE_INPUT_MAX - 1);
    history[0][CONSOLE_INPUT_MAX - 1] = '\0';
    
    history_index = -1;
}

/**
 * @brief Process and execute command
 */
static void console_process_command(const char* cmd) {
    /* Skip empty commands */
    if (cmd == NULL || cmd[0] == '\0') {
        return;
    }
    
    /* Add to history */
    console_add_history(cmd);
    
    /* Execute */
    console_execute_cmd(cmd);
}

/**
 * @brief Find and execute command
 */
static void console_execute_cmd(const char* cmd) {
    char cmd_copy[CONSOLE_INPUT_MAX];
    char* args;
    
    strncpy(cmd_copy, cmd, CONSOLE_INPUT_MAX - 1);
    cmd_copy[CONSOLE_INPUT_MAX - 1] = '\0';
    
    /* Find arguments */
    args = strchr(cmd_copy, ' ');
    if (args) {
        *args = '\0';
        args++;
        /* Skip leading spaces */
        while (*args == ' ') args++;
    }
    
    /* Search command table */
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(cmd_copy, commands[i].name) == 0) {
            commands[i].handler(args ? args : "");
            return;
        }
    }
    
    /* Unknown command */
    vga_printf(vga_get_cursor_row() + 1, 5, COLOR_RED, COLOR_BLACK,
              "Unknown command: %s", cmd_copy);
    serial_puts("Unknown command: ");
    serial_puts(cmd_copy);
    serial_puts("\r\n");
}

/**
 * @brief Command: ver - Show version
 */
static void cmd_ver(const char* args) {
    (void)args;
    
    vga_printf(vga_get_cursor_row(), 5, COLOR_WHITE, COLOR_BLACK,
              "KOS Bootloader v0.1-alpha");
    vga_printf(vga_get_cursor_row() + 1, 5, COLOR_LIGHT_GRAY, COLOR_BLACK,
              "Build: " __DATE__ " " __TIME__);
    vga_printf(vga_get_cursor_row() + 2, 5, COLOR_LIGHT_GRAY, COLOR_BLACK,
              "Target: i686, BIOS");
    
    serial_puts("KOS Bootloader v0.1-alpha\r\n");
    serial_puts("Build: " __DATE__ " " __TIME__ "\r\n");
    serial_puts("Target: i686, BIOS\r\n\r\n");
    
    vga_set_cursor(vga_get_cursor_row() + 3, 5);
}

/**
 * @brief Command: info - Show system info
 */
static void cmd_info(const char* args) {
    (void)args;
    
    iso_mount_t* mount = iso_get_mount_info();
    
    vga_puts("System Information:\r\n");
    vga_printf(vga_get_cursor_row(), 5, COLOR_WHITE, COLOR_BLACK,
              "Memory: 0x000A0000-0x000FFFFF [ROM]");
    vga_printf(vga_get_cursor_row() + 1, 5, COLOR_WHITE, COLOR_BLACK,
              "       0x00100000+ [RAM]");
    
    if (mount && mount->mounted) {
        vga_printf(vga_get_cursor_row() + 2, 5, COLOR_WHITE, COLOR_BLACK,
                  "ISO9660: volume \"%s\"", mount->volume_id);
        vga_printf(vga_get_cursor_row() + 3, 5, COLOR_LIGHT_GRAY, COLOR_BLACK,
                  "         %u sectors (%u MB)", 
                  mount->total_sectors,
                  (mount->total_sectors * ISO_SECTOR_SIZE) / (1024 * 1024));
    } else {
        vga_printf(vga_get_cursor_row() + 2, 5, COLOR_YELLOW, COLOR_BLACK,
                  "ISO9660: not mounted");
    }
    
    serial_puts("\r\nSystem Information:\r\n");
    serial_puts("  Memory: 0x000A0000-0x000FFFFF [ROM]\r\n");
    serial_puts("          0x00100000+ [RAM]\r\n");
    
    if (mount && mount->mounted) {
        serial_puts("  ISO9660: volume \"");
        serial_puts(mount->volume_id);
        serial_puts("\"\r\n");
    }
    serial_puts("\r\n");
}

/**
 * @brief Command: help - Show help
 */
static void cmd_help(const char* args) {
    (void)args;
    
    vga_printf(vga_get_cursor_row(), 5, COLOR_WHITE, COLOR_BLACK,
              "Available commands:");
    
    for (int i = 0; commands[i].name != NULL; i++) {
        vga_printf(vga_get_cursor_row() + 1, 7, COLOR_LIGHT_GRAY, COLOR_BLACK,
                  "%-8s - %s", commands[i].name, commands[i].description);
    }
    
    serial_puts("\r\nAvailable commands:\r\n");
    for (int i = 0; commands[i].name != NULL; i++) {
        serial_puts("  ");
        serial_puts(commands[i].name);
        serial_puts(" - ");
        serial_puts(commands[i].description);
        serial_puts("\r\n");
    }
    serial_puts("\r\n");
}

/**
 * @brief Command: exec - Execute .COM file
 */
static void cmd_exec(const char* args) {
    if (args == NULL || args[0] == '\0') {
        vga_printf(vga_get_cursor_row(), 5, COLOR_RED, COLOR_BLACK,
                  "Usage: exec <filename.com>");
        serial_puts("Usage: exec <filename.com>\r\n\r\n");
        return;
    }
    
    vga_printf(vga_get_cursor_row(), 5, COLOR_WHITE, COLOR_BLACK,
              "[Loading %s...]", args);
    serial_puts("[Loading ");
    serial_puts(args);
    serial_puts("...]\r\n");
    
    /* TODO: Implement .COM file loading and execution */
    vga_printf(vga_get_cursor_row() + 1, 5, COLOR_YELLOW, COLOR_BLACK,
              "[Not implemented in this version]");
    serial_puts("[Not implemented in this version]\r\n\r\n");
}

/**
 * @brief Command: mem - Show memory map
 */
static void cmd_mem(const char* args) {
    (void)args;
    
    vga_printf(vga_get_cursor_row(), 5, COLOR_WHITE, COLOR_BLACK,
              "Memory Map:");
    vga_printf(vga_get_cursor_row() + 1, 7, COLOR_LIGHT_GRAY, COLOR_BLACK,
              "0x00000000-0x0009FFFF: Conventional Memory (640KB)");
    vga_printf(vga_get_cursor_row() + 2, 7, COLOR_LIGHT_GRAY, COLOR_BLACK,
              "0x000A0000-0x000BFFFF: VGA Framebuffer (128KB)");
    vga_printf(vga_get_cursor_row() + 3, 7, COLOR_LIGHT_GRAY, COLOR_BLACK,
              "0x000C0000-0x000FFFFF: ROM/BIOS Area (256KB)");
    vga_printf(vga_get_cursor_row() + 4, 7, COLOR_LIGHT_GRAY, COLOR_BLACK,
              "0x00100000-0xFFFFFFFF: Extended Memory");
    
    serial_puts("\r\nMemory Map:\r\n");
    serial_puts("  0x00000000-0x0009FFFF: Conventional Memory\r\n");
    serial_puts("  0x000A0000-0x000BFFFF: VGA Framebuffer\r\n");
    serial_puts("  0x000C0000-0x000FFFFF: ROM/BIOS\r\n");
    serial_puts("  0x00100000+:           Extended Memory\r\n\r\n");
}

/**
 * @brief Command: reboot - Reboot system
 */
static void cmd_reboot(const char* args) {
    (void)args;
    
    vga_print_string(vga_get_cursor_row(), 5, 
                    "Rebooting...", COLOR_YELLOW, COLOR_BLACK);
    serial_puts("\r\nRebooting...\r\n");
    
    /* Trigger reboot via keyboard controller */
    serial_flush();
    
    for (volatile int i = 0; i < 100000; i++);
    
    /* Triple fault or keyboard controller reset */
    outb(0x64, 0xFE);  /* Keyboard controller reset */
    
    /* If that doesn't work, triple fault */
    asm volatile(
        "lidt word ptr [0]\n\t"
        "int3\n\t"
    );
    
    /* Should never reach here */
    while (1) {
        hlt();
    }
}

/**
 * @brief Main entry point for recovery console
 */
void recovery_console_main(void) {
    console_init();
    console_display_banner();
    
    while (1) {
        console_print_prompt();
        
        if (console_read_line(input_buffer, CONSOLE_INPUT_MAX)) {
            console_process_command(input_buffer);
        }
        
        input_pos = 0;
        memset(input_buffer, 0, CONSOLE_INPUT_MAX);
    }
}
