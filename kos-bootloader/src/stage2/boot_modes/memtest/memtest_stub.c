/* ============================================================================
 * KOS Bootloader - Memory Test Stub Implementation
 * ============================================================================
 * Simple memory pattern test for bootloader environment
 * Tests memory from 1MB to available limit
 * ============================================================================
 */

#include "memtest_stub.h"
#include "vga.h"
#include "serial.h"
#include "keyboard.h"
#include "printf.h"
#include "io.h"

/* Memory test configuration */
#define MEMTEST_START_ADDR  0x00100000  /* Start at 1MB */
#define MEMTEST_END_ADDR    0x00800000  /* End at 8MB (safe limit) */
#define MEMTEST_BLOCK_SIZE  4096        /* Test block size */

/* Test patterns */
static const uint32_t test_patterns[] = {
    0x00000000,  /* All zeros */
    0xFFFFFFFF,  /* All ones */
    0xAAAAAAAA,  /* Alternating bits */
    0x55555555,  /* Alternating bits inverse */
    0xDEADBEEF,  /* Random pattern 1 */
    0xCAFEBABE,  /* Random pattern 2 */
};

#define NUM_PATTERNS (sizeof(test_patterns) / sizeof(test_patterns[0]))

/**
 * @brief Display memory test header
 */
static void display_header(void) {
    vga_clear_screen(COLOR_BLACK);
    vga_print_string_centered(2, "KOS Memory Test", COLOR_WHITE, COLOR_BLUE);
    vga_print_string_centered(3, "v0.1-alpha", COLOR_LIGHT_GRAY, COLOR_BLUE);
    
    vga_print_string(6, 5, "Testing memory range:", COLOR_WHITE, COLOR_BLACK);
    vga_printf(7, 7, COLOR_LIGHT_GRAY, COLOR_BLACK,
              "Start: 0x%08X (%u MB)", MEMTEST_START_ADDR, 
              MEMTEST_START_ADDR / (1024 * 1024));
    vga_printf(8, 7, COLOR_LIGHT_GRAY, COLOR_BLACK,
              "End:   0x%08X (%u MB)", MEMTEST_END_ADDR,
              MEMTEST_END_ADDR / (1024 * 1024));
    
    serial_puts("\r\n[KOS] Memory Test starting...\r\n");
}

/**
 * @brief Display test progress
 */
static void display_progress(uint32_t current, uint32_t total, int pattern_idx) {
    int percent = (current * 100) / total;
    
    vga_draw_progress_bar(10, 10, 40, percent, COLOR_GREEN);
    vga_printf(11, 20, COLOR_WHITE, COLOR_BLACK,
              "Progress: %d%% (%u/%u KB)", percent, 
              current / 1024, total / 1024);
    vga_printf(12, 20, COLOR_LIGHT_GRAY, COLOR_BLACK,
              "Pattern %d/%d: 0x%08X", pattern_idx + 1, NUM_PATTERNS,
              test_patterns[pattern_idx]);
}

/**
 * @brief Report error at address
 */
static void report_error(uint32_t addr, uint32_t expected, uint32_t actual) {
    static int error_count = 0;
    static int last_error_row = 15;
    
    error_count++;
    
    vga_printf(last_error_row, 5, COLOR_RED, COLOR_BLACK,
              "ERR@%08X: exp=%08X got=%08X", addr, expected, actual);
    
    serial_puts("[MEMTEST] ERROR at 0x");
    serial_puthex(addr, 8);
    serial_puts(": expected 0x");
    serial_puthex(expected, 8);
    serial_puts(", got 0x");
    serial_puthex(actual, 8);
    serial_puts("\r\n");
    
    last_error_row++;
    if (last_error_row >= 23) {
        last_error_row = 15;
    }
}

/**
 * @brief Run single pattern test on memory block
 */
static bool test_pattern(uint32_t* start, uint32_t* end, uint32_t pattern) {
    volatile uint32_t* ptr;
    
    /* Write pattern */
    for (ptr = start; ptr < end; ptr++) {
        *ptr = pattern;
    }
    
    /* Verify pattern */
    for (ptr = start; ptr < end; ptr++) {
        if (*ptr != pattern) {
            report_error((uint32_t)ptr, pattern, *ptr);
            return false;
        }
    }
    
    return true;
}

/**
 * @brief Run all memory tests
 */
void memtest_main(void) {
    uint32_t* start = (uint32_t*)MEMTEST_START_ADDR;
    uint32_t* end = (uint32_t*)MEMTEST_END_ADDR;
    uint32_t total_bytes = MEMTEST_END_ADDR - MEMTEST_START_ADDR;
    uint32_t tested_bytes = 0;
    int errors = 0;
    bool passed;
    
    display_header();
    
    serial_puts("[MEMTEST] Starting pattern tests...\r\n");
    
    /* Run each pattern */
    for (size_t p = 0; p < NUM_PATTERNS; p++) {
        tested_bytes = 0;
        
        while (tested_bytes < total_bytes) {
            uint32_t* block_start = start + (tested_bytes / sizeof(uint32_t));
            uint32_t* block_end = block_start + (MEMTEST_BLOCK_SIZE / sizeof(uint32_t));
            
            if ((uint8_t*)block_end > (uint8_t*)end) {
                block_end = end;
            }
            
            passed = test_pattern(block_start, block_end, test_patterns[p]);
            
            if (!passed) {
                errors++;
            }
            
            tested_bytes += MEMTEST_BLOCK_SIZE;
            
            display_progress(tested_bytes, total_bytes, p);
            
            /* Check for user abort */
            if (keyboard_check_key()) {
                uint8_t key = keyboard_get_key();
                if (key == SCAN_ESCAPE) {
                    vga_print_string(20, 20, "Test aborted by user", 
                                   COLOR_YELLOW, COLOR_BLACK);
                    serial_puts("[MEMTEST] Aborted by user\r\n");
                    goto done;
                }
            }
        }
    }
    
done:
    /* Display final results */
    vga_print_string(20, 10, "Memory Test Complete", COLOR_WHITE, COLOR_BLUE);
    
    if (errors == 0) {
        vga_print_string(21, 15, "RESULT: ALL TESTS PASSED", COLOR_GREEN, COLOR_BLACK);
        serial_puts("[MEMTEST] All tests passed!\r\n");
    } else {
        vga_printf(21, 15, COLOR_RED, COLOR_BLACK,
                  "RESULT: %d ERRORS FOUND", errors);
        serial_puts("[MEMTEST] Tests completed with errors\r\n");
    }
    
    vga_print_string(23, 15, "Press any key to continue...", 
                    COLOR_LIGHT_GRAY, COLOR_BLACK);
    
    /* Wait for keypress */
    while (!keyboard_check_key()) {
        hlt();
    }
    keyboard_get_key();  /* Consume key */
}
