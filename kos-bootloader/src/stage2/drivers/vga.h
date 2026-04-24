/* ============================================================================
 * KOS Bootloader - VGA Driver (Mode 0x12)
 * ============================================================================
 * VGA graphics mode driver: 640x480x16 colors
 * Linear framebuffer at 0xA0000
 * ============================================================================
 */

#ifndef KOS_VGA_H
#define KOS_VGA_H

#include "types.h"

/* ============================================================================
 * Constants and Definitions
 * ============================================================================
 */

/* VGA framebuffer physical address */
#define VGA_FRAMEBUFFER_ADDR    0xA0000

/* Screen dimensions for mode 0x12 */
#define VGA_WIDTH               640
#define VGA_HEIGHT              480
#define VGA_BPP                 4       /* 4 bits per pixel (16 colors) */
#define VGA_BYTES_PER_PIXEL     (VGA_BPP / 8)
#define VGA_PITCH               (VGA_WIDTH * VGA_BYTES_PER_PIXEL)
#define VGA_BUFFER_SIZE         (VGA_WIDTH * VGA_HEIGHT * VGA_BYTES_PER_PIXEL)

/* VGA color definitions */
typedef enum {
    COLOR_BLACK = 0,
    COLOR_BLUE = 1,
    COLOR_GREEN = 2,
    COLOR_CYAN = 3,
    COLOR_RED = 4,
    COLOR_MAGENTA = 5,
    COLOR_BROWN = 6,
    COLOR_LIGHT_GRAY = 7,
    COLOR_DARK_GRAY = 8,
    COLOR_LIGHT_BLUE = 9,
    COLOR_LIGHT_GREEN = 10,
    COLOR_LIGHT_CYAN = 11,
    COLOR_LIGHT_RED = 12,
    COLOR_LIGHT_MAGENTA = 13,
    COLOR_YELLOW = 14,
    COLOR_WHITE = 15
} vga_color_t;

/* VGA font dimensions (8x16 default) */
#define FONT_WIDTH              8
#define FONT_HEIGHT             16
#define CHARS_PER_ROW           (VGA_WIDTH / FONT_WIDTH)
#define CHARS_PER_COL           (VGA_HEIGHT / FONT_HEIGHT)

/* ============================================================================
 * Function Prototypes
 * ============================================================================
 */

/**
 * @brief Initialize VGA mode 0x12 (640x480x16)
 * Sets up linear framebuffer and clears screen to black
 */
void vga_init(void);

/**
 * @brief Clear the entire screen with specified color
 * @param color Background color (0-15)
 */
void vga_clear_screen(vga_color_t color);

/**
 * @brief Set a single pixel at specified coordinates
 * @param x X coordinate (0 to VGA_WIDTH-1)
 * @param y Y coordinate (0 to VGA_HEIGHT-1)
 * @param color Pixel color (0-15)
 */
void vga_set_pixel(int x, int y, vga_color_t color);

/**
 * @brief Get pixel color at specified coordinates
 * @param x X coordinate
 * @param y Y coordinate
 * @return Color value at pixel
 */
vga_color_t vga_get_pixel(int x, int y);

/**
 * @brief Draw a horizontal line
 * @param x0 Starting X coordinate
 * @param y Y coordinate
 * @param width Line width in pixels
 * @param color Line color
 */
void vga_draw_hline(int x0, int y, int width, vga_color_t color);

/**
 * @brief Draw a vertical line
 * @param x X coordinate
 * @param y0 Starting Y coordinate
 * @param height Line height in pixels
 * @param color Line color
 */
void vga_draw_vline(int x, int y0, int height, vga_color_t color);

/**
 * @brief Draw a rectangle outline
 * @param x Top-left X coordinate
 * @param y Top-left Y coordinate
 * @param width Rectangle width
 * @param height Rectangle height
 * @param color Border color
 */
void vga_draw_rect(int x, int y, int width, int height, vga_color_t color);

/**
 * @brief Fill a rectangle with solid color
 * @param x Top-left X coordinate
 * @param y Top-left Y coordinate
 * @param width Rectangle width
 * @param height Rectangle height
 * @param color Fill color
 */
void vga_fill_rect(int x, int y, int width, int height, vga_color_t color);

/**
 * @brief Draw a character at specified row/column
 * @param row Text row (0 to CHARS_PER_COL-1)
 * @param col Text column (0 to CHARS_PER_ROW-1)
 * @param c Character to draw
 * @param fg Foreground color
 * @param bg Background color
 */
void vga_draw_char(int row, int col, char c, vga_color_t fg, vga_color_t bg);

/**
 * @brief Print a string at specified row/column
 * @param row Text row
 * @param col Text column
 * @param str Null-terminated string to print
 * @param fg Foreground color
 * @param bg Background color
 */
void vga_print_string(int row, int col, const char* str, 
                      vga_color_t fg, vga_color_t bg);

/**
 * @brief Print a centered string at specified row
 * @param row Text row
 * @param str Null-terminated string to print
 * @param fg Foreground color
 * @param bg Background color
 */
void vga_print_string_centered(int row, const char* str,
                               vga_color_t fg, vga_color_t bg);

/**
 * @brief Print formatted string (printf-like)
 * @param row Text row
 * @param col Text column
 * @param fg Foreground color
 * @param bg Background color
 * @param format Format string
 * @param ... Format arguments
 */
void vga_printf(int row, int col, vga_color_t fg, vga_color_t bg,
                const char* format, ...) __attribute__((format(printf, 5, 6)));

/**
 * @brief Scroll screen up by one line
 */
void vga_scroll_up(void);

/**
 * @brief Move cursor to next position, scrolling if needed
 */
void vga_advance_cursor(void);

/**
 * @brief Set current cursor position
 * @param row Text row
 * @param col Text column
 */
void vga_set_cursor(int row, int col);

/**
 * @brief Get current cursor row
 * @return Current cursor row
 */
int vga_get_cursor_row(void);

/**
 * @brief Get current cursor column
 * @return Current cursor column
 */
int vga_get_cursor_col(void);

/**
 * @brief Draw a simple progress bar
 * @param x X coordinate
 * @param y Y coordinate
 * @param width Bar width in characters
 * @param progress Progress value (0-100)
 * @param color Bar color
 */
void vga_draw_progress_bar(int x, int y, int width, int progress, 
                           vga_color_t color);

/**
 * @brief Get pointer to VGA framebuffer
 * @return Pointer to framebuffer memory
 */
uint8_t* vga_get_framebuffer(void);

#endif /* KOS_VGA_H */
