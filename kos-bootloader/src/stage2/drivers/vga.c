/* ============================================================================
 * KOS Bootloader - VGA Driver Implementation (Mode 0x12)
 * ============================================================================
 * VGA graphics mode driver: 640x480x16 colors
 * Linear framebuffer at 0xA0000
 * ============================================================================
 */

#include "vga.h"
#include "io.h"
#include "string.h"
#include "printf.h"

/* ============================================================================
 * Static Variables
 * ============================================================================
 */

static uint8_t* framebuffer = (uint8_t*)VGA_FRAMEBUFFER_ADDR;
static int cursor_row = 0;
static int cursor_col = 0;

/* ============================================================================
 * Font Data (8x16 VGA font, subset of ASCII characters)
 * ============================================================================
 */

static const uint8_t font_data[128][16] = {
    /* Space (0x20) - all zeros */
    [0x20] = {0},
    
    /* Exclamation mark (0x21) */
    [0x21] = {
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00,
        0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    },
    
    /* Numbers 0-9 (0x30-0x39) */
    [0x30] = {  /* '0' */
        0x7E, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
        0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x7E
    },
    [0x31] = {  /* '1' */
        0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7E
    },
    [0x32] = {  /* '2' */
        0x7E, 0x81, 0x81, 0x01, 0x02, 0x04, 0x08, 0x10,
        0x20, 0x40, 0x80, 0x80, 0x80, 0x80, 0x80, 0xFF
    },
    [0x33] = {  /* '3' */
        0x7E, 0x81, 0x81, 0x01, 0x01, 0x3E, 0x40, 0x80,
        0x80, 0x01, 0x01, 0x01, 0x81, 0x81, 0x81, 0x7E
    },
    [0x34] = {  /* '4' */
        0x08, 0x18, 0x38, 0x68, 0xC8, 0x88, 0x88, 0x88,
        0x88, 0xFF, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88
    },
    [0x35] = {  /* '5' */
        0xFF, 0x80, 0x80, 0x80, 0x80, 0xFC, 0x82, 0x81,
        0x80, 0x80, 0x80, 0x80, 0x81, 0x81, 0x81, 0x7E
    },
    [0x36] = {  /* '6' */
        0x3C, 0x60, 0x80, 0x80, 0x80, 0xFC, 0x82, 0x81,
        0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x7E
    },
    [0x37] = {  /* '7' */
        0xFF, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
        0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
    },
    [0x38] = {  /* '8' */
        0x7E, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x7E,
        0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x7E
    },
    [0x39] = {  /* '9' */
        0x7E, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFE,
        0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7E
    },
    
    /* Uppercase letters A-Z (0x41-0x5A) */
    [0x41] = {  /* 'A' */
        0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x7E,
        0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66
    },
    [0x42] = {  /* 'B' */
        0xFC, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0xFC,
        0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0xFC
    },
    [0x43] = {  /* 'C' */
        0x3C, 0x66, 0x82, 0x80, 0x80, 0x80, 0x80, 0x80,
        0x80, 0x80, 0x80, 0x80, 0x82, 0x66, 0x3C, 0x00
    },
    /* ... more characters would be defined here ... */
};

/* Simple fallback font for basic ASCII */
static const uint8_t simple_font[95][8] = {
    /* Default pattern for undefined characters */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

/* ============================================================================
 * Internal Helper Functions
 * ============================================================================
 */

/**
 * @brief Get pixel offset in framebuffer
 */
static inline uint32_t get_pixel_offset(int x, int y) {
    return (y * VGA_PITCH) + (x * VGA_BYTES_PER_PIXEL);
}

/**
 * @brief Draw a character using simple bitmap font
 */
static void draw_char_bitmap(int row, int col, char c, 
                             vga_color_t fg, vga_color_t bg) {
    int x = col * FONT_WIDTH;
    int y = row * FONT_HEIGHT;
    
    /* Get font data for character */
    const uint8_t* char_data;
    if (c >= 32 && c < 127) {
        /* Use simple 8x8 font pattern */
        uint8_t pattern = 0;
        switch (c) {
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                pattern = 0xFF;  /* Solid block for demo */
                break;
            default:
                pattern = 0xAA;  /* Checkerboard for others */
                break;
        }
        
        /* For now, draw simple representation */
        vga_fill_rect(x, y, FONT_WIDTH, FONT_HEIGHT, bg);
        
        /* Draw character outline or pattern */
        if (pattern != 0) {
            vga_draw_rect(x + 1, y + 1, FONT_WIDTH - 2, FONT_HEIGHT - 2, fg);
        }
    } else {
        /* Unknown character - draw box */
        vga_fill_rect(x, y, FONT_WIDTH, FONT_HEIGHT, bg);
        vga_draw_rect(x, y, FONT_WIDTH, FONT_HEIGHT, fg);
    }
}

/* ============================================================================
 * Public API Implementation
 * ============================================================================
 */

void vga_init(void) {
    /* Set VGA mode 0x12 via BIOS interrupt */
    asm volatile(
        "mov $0x4F02, %ax\n\t"      /* VESA function: set mode */
        "mov $0x4112, %bx\n\t"      /* Mode 0x112 (640x480x16, linear) */
        "int $0x10\n\t"
        :
        :
        : "eax", "ebx", "memory"
    );
    
    /* Fallback: Try standard mode 0x12 if VESA fails */
    asm volatile(
        "mov $0x0012, %ax\n\t"
        "int $0x10\n\t"
        :
        :
        : "eax", "memory"
    );
    
    /* Initialize framebuffer pointer */
    framebuffer = (uint8_t*)VGA_FRAMEBUFFER_ADDR;
    
    /* Reset cursor position */
    cursor_row = 0;
    cursor_col = 0;
    
    /* Clear screen to black */
    vga_clear_screen(COLOR_BLACK);
}

void vga_clear_screen(vga_color_t color) {
    /* Create fill pattern with color in both nibbles */
    uint8_t pattern = (color << 4) | color;
    
    /* Fill entire framebuffer with pattern */
    memset(framebuffer, pattern, VGA_BUFFER_SIZE);
    
    /* Reset cursor */
    cursor_row = 0;
    cursor_col = 0;
}

void vga_set_pixel(int x, int y, vga_color_t color) {
    /* Bounds checking */
    if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT) {
        return;
    }
    
    uint32_t offset = get_pixel_offset(x, y);
    uint8_t* pixel = &framebuffer[offset];
    
    /* Each byte contains two pixels (4 bits each) */
    if (x % 2 == 0) {
        /* Even pixel (high nibble) */
        *pixel = (*pixel & 0x0F) | (color << 4);
    } else {
        /* Odd pixel (low nibble) */
        *pixel = (*pixel & 0xF0) | (color & 0x0F);
    }
}

vga_color_t vga_get_pixel(int x, int y) {
    if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT) {
        return COLOR_BLACK;
    }
    
    uint32_t offset = get_pixel_offset(x, y);
    uint8_t* pixel = &framebuffer[offset];
    
    if (x % 2 == 0) {
        return (vga_color_t)((*pixel >> 4) & 0x0F);
    } else {
        return (vga_color_t)(*pixel & 0x0F);
    }
}

void vga_draw_hline(int x0, int y, int width, vga_color_t color) {
    for (int i = 0; i < width && x0 + i < VGA_WIDTH; i++) {
        vga_set_pixel(x0 + i, y, color);
    }
}

void vga_draw_vline(int x, int y0, int height, vga_color_t color) {
    for (int i = 0; i < height && y0 + i < VGA_HEIGHT; i++) {
        vga_set_pixel(x, y0 + i, color);
    }
}

void vga_draw_rect(int x, int y, int width, int height, vga_color_t color) {
    /* Top and bottom edges */
    vga_draw_hline(x, y, width, color);
    vga_draw_hline(x, y + height - 1, width, color);
    
    /* Left and right edges */
    vga_draw_vline(x, y, height, color);
    vga_draw_vline(x + width - 1, y, height, color);
}

void vga_fill_rect(int x, int y, int width, int height, vga_color_t color) {
    for (int row = 0; row < height && y + row < VGA_HEIGHT; row++) {
        vga_draw_hline(x, y + row, width, color);
    }
}

void vga_draw_char(int row, int col, char c, vga_color_t fg, vga_color_t bg) {
    /* Bounds checking */
    if (row < 0 || row >= CHARS_PER_COL || col < 0 || col >= CHARS_PER_ROW) {
        return;
    }
    
    draw_char_bitmap(row, col, c, fg, bg);
}

void vga_print_string(int row, int col, const char* str,
                      vga_color_t fg, vga_color_t bg) {
    if (str == NULL) {
        return;
    }
    
    int current_col = col;
    
    while (*str) {
        if (*str == '\n') {
            /* Newline: move to next row, reset column */
            row++;
            current_col = col;
            
            /* Scroll if needed */
            if (row >= CHARS_PER_COL) {
                vga_scroll_up();
                row--;
            }
        } else if (*str == '\r') {
            /* Carriage return: reset column */
            current_col = col;
        } else {
            /* Regular character */
            vga_draw_char(row, current_col, *str, fg, bg);
            current_col++;
            
            /* Wrap to next line if at edge */
            if (current_col >= CHARS_PER_ROW) {
                row++;
                current_col = col;
                
                if (row >= CHARS_PER_COL) {
                    vga_scroll_up();
                    row--;
                }
            }
        }
        str++;
    }
    
    /* Update cursor position */
    cursor_row = row;
    cursor_col = current_col;
}

void vga_print_string_centered(int row, const char* str,
                               vga_color_t fg, vga_color_t bg) {
    if (str == NULL) {
        return;
    }
    
    int len = strlen(str);
    int col = (CHARS_PER_ROW - len) / 2;
    
    if (col < 0) {
        col = 0;
    }
    
    vga_print_string(row, col, str, fg, bg);
}

void vga_printf(int row, int col, vga_color_t fg, vga_color_t bg,
                const char* format, ...) {
    char buffer[256];
    va_list args;
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    vga_print_string(row, col, buffer, fg, bg);
}

void vga_scroll_up(void) {
    /* Copy each row up by one */
    uint8_t* src = framebuffer + (FONT_HEIGHT * VGA_PITCH);
    uint8_t* dst = framebuffer;
    size_t size = VGA_BUFFER_SIZE - (FONT_HEIGHT * VGA_PITCH);
    
    memmove(dst, src, size);
    
    /* Clear the last row */
    vga_fill_rect(0, VGA_HEIGHT - FONT_HEIGHT, 
                  VGA_WIDTH, FONT_HEIGHT, COLOR_BLACK);
    
    /* Adjust cursor */
    if (cursor_row > 0) {
        cursor_row--;
    }
}

void vga_advance_cursor(void) {
    cursor_col++;
    
    if (cursor_col >= CHARS_PER_ROW) {
        cursor_col = 0;
        cursor_row++;
        
        if (cursor_row >= CHARS_PER_COL) {
            vga_scroll_up();
        }
    }
}

void vga_set_cursor(int row, int col) {
    if (row >= 0 && row < CHARS_PER_COL) {
        cursor_row = row;
    }
    if (col >= 0 && col < CHARS_PER_ROW) {
        cursor_col = col;
    }
}

int vga_get_cursor_row(void) {
    return cursor_row;
}

int vga_get_cursor_col(void) {
    return cursor_col;
}

void vga_draw_progress_bar(int x, int y, int width, int progress,
                           vga_color_t color) {
    /* Clamp progress to 0-100 */
    if (progress < 0) progress = 0;
    if (progress > 100) progress = 100;
    
    /* Calculate filled width */
    int filled = (width * progress) / 100;
    
    /* Draw bar background */
    vga_draw_rect(x * FONT_WIDTH, y * FONT_HEIGHT, 
                  width * FONT_WIDTH, FONT_HEIGHT, COLOR_LIGHT_GRAY);
    
    /* Draw filled portion */
    if (filled > 0) {
        vga_fill_rect(x * FONT_WIDTH + 1, y * FONT_HEIGHT + 1,
                      filled * FONT_WIDTH - 2, FONT_HEIGHT - 2, color);
    }
}

uint8_t* vga_get_framebuffer(void) {
    return framebuffer;
}
