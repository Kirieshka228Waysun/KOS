/* ============================================================================
 * KOS Bootloader - Test Kernel
 * ============================================================================
 * Minimal test kernel that prints "KOS OK" and halts
 * For testing bootloader functionality
 * ============================================================================
 */

#include <stdint.h>

/* VGA framebuffer for mode 0x12 */
#define VGA_FRAMEBUFFER  0xA0000
#define VGA_WIDTH        640
#define VGA_HEIGHT       480

/* Unused attribute macro (local definition since we can't include types.h) */
#define UNUSED __attribute__((unused))

/* Fill memory with value */
static void memset_byte(void* dst, unsigned char val, unsigned int count) {
    unsigned char* p = (unsigned char*)dst;
    while (count--) {
        *p++ = val;
    }
}

/* Draw a character (simple 8x8 block font) */
static void draw_char(int x, int y, char c UNUSED, unsigned char color) {
    unsigned char* fb = (unsigned char*)VGA_FRAMEBUFFER;
    
    /* For simplicity, just draw colored blocks for characters */
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            int px = x + col;
            int py = y + row;
            if (px < VGA_WIDTH && py < VGA_HEIGHT) {
                unsigned int offset = (py * VGA_WIDTH / 2) + (px / 2);
                if (px % 2 == 0) {
                    fb[offset] = (fb[offset] & 0x0F) | (color << 4);
                } else {
                    fb[offset] = (fb[offset] & 0xF0) | (color & 0x0F);
                }
            }
        }
    }
}

/* Print string at character position */
static void print_at(int row, int col, const char* str, unsigned char color) {
    int x = col * 8;
    int y = row * 8;
    
    while (*str) {
        if (*str != ' ') {
            draw_char(x, y, *str, color);
        }
        str++;
        x += 8;
    }
}

/* I/O port output */
static void outb(unsigned short port, unsigned char val) {
    asm volatile("outb %0, %w1" : : "a"(val), "Nd"(port));
}

/* Serial port initialization */
static void serial_init(void) {
    outb(0x3F8 + 1, 0x00);    /* Disable interrupts */
    outb(0x3F8 + 3, 0x80);    /* Enable DLAB */
    outb(0x3F8 + 0, 0x0C);    /* Set divisor (9600 baud) */
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x03);    /* 8 bits, no parity, one stop bit */
    outb(0x3F8 + 2, 0xC7);    /* Enable FIFO */
    outb(0x3F8 + 4, 0x0B);    /* IRQs enabled, RTS/DSR set */
}

/* Read from I/O port - forward declaration */
static unsigned char inb(unsigned short port);

/* Send character via serial */
static void serial_putc(char c) {
    while ((inb(0x3F8 + 5) & 0x20) == 0);  /* Wait for THRE */
    outb(0x3F8, c);
    if (c == '\n') {
        serial_putc('\r');
    }
}

/* Send string via serial */
static void serial_puts(const char* str) {
    while (*str) {
        serial_putc(*str++);
    }
}

/* Read from I/O port */
static unsigned char inb(unsigned short port) {
    unsigned char val;
    asm volatile("inb %w1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/* Halt CPU */
static void halt(void) {
    asm volatile("cli; hlt");
}

/* ============================================================================
 * Kernel Entry Point
 * ============================================================================
 */
void kernel_main(void) {
    /* Initialize serial */
    serial_init();
    
    /* Clear screen (fill with black) */
    memset_byte((void*)VGA_FRAMEBUFFER, 0x00, VGA_WIDTH * VGA_HEIGHT / 2);
    
    /* Print welcome message */
    const char* msg1 = "KOS KERNEL LOADED";
    const char* msg2 = "Address: 0x00010000";
    const char* msg3 = "Status: OK";
    
    print_at(10, 24, msg1, 0x0F);  /* White on black */
    print_at(12, 26, msg2, 0x07);  /* Light gray on black */
    print_at(14, 27, msg3, 0x0A);  /* Green on black */
    
    /* Print to serial */
    serial_puts("\r\n");
    serial_puts("========================================\r\n");
    serial_puts("KOS KERNEL STARTED\r\n");
    serial_puts("========================================\r\n");
    serial_puts("Loaded at: 0x00010000\r\n");
    serial_puts("Status: OK\r\n");
    serial_puts("Press any key to halt...\r\n");
    serial_puts("========================================\r\n");
    
    /* Wait for keypress then halt */
    while (1) {
        /* Check for keyboard input */
        if ((inb(0x64) & 0x01) != 0) {
            inb(0x60);  /* Read scancode */
            break;
        }
        
        halt();
    }
    
    /* Final halt */
    serial_puts("\r\nHalting...\r\n");
    while (1) {
        halt();
    }
}

/* Entry point alias */
void _start(void) __attribute__((alias("kernel_main")));
