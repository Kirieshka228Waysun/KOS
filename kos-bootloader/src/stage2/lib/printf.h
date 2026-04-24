/* ============================================================================
 * KOS Bootloader - Printf Implementation
 * ============================================================================
 * Minimal printf/vsnprintf implementation (no libc dependency)
 * Supports: %d, %u, %x, %X, %p, %s, %c, %%
 * ============================================================================
 */

#ifndef KOS_PRINTF_H
#define KOS_PRINTF_H

#include "types.h"
#include <stdarg.h>

/**
 * @brief Format and print to buffer
 * @param buf Output buffer
 * @param size Buffer size
 * @param format Format string
 * @param args Variable arguments
 * @return Number of characters written (excluding null terminator)
 */
int vsnprintf(char* buf, size_t size, const char* format, va_list args);

/**
 * @brief Format and print to buffer
 * @param buf Output buffer
 * @param size Buffer size
 * @param format Format string
 * @param ... Format arguments
 * @return Number of characters written (excluding null terminator)
 */
int snprintf(char* buf, size_t size, const char* format, ...) 
    __attribute__((format(printf, 3, 4)));

/**
 * @brief Format and print (output function provided by caller)
 * @param putc_fn Function to output a character
 * @param arg Argument passed to putc_fn
 * @param format Format string
 * @param args Variable arguments
 * @return Number of characters printed
 */
typedef void (*putc_func_t)(char c, void* arg);
int vprintf_custom(putc_func_t putc_fn, void* arg, 
                   const char* format, va_list args);

#endif /* KOS_PRINTF_H */
