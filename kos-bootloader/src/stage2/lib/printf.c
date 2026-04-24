/* ============================================================================
 * KOS Bootloader - Printf Implementation
 * ============================================================================
 * Minimal printf/vsnprintf implementation (no libc dependency)
 * Supports: %d, %u, %x, %X, %p, %s, %c, %%
 * ============================================================================
 */

#include "printf.h"
#include "string.h"

/**
 * @brief Put a character into buffer with bounds checking
 * Note: This function is kept for API compatibility but not used directly
 */
__attribute__((unused))
static void buf_putc(char c, void* arg) {
    char** buf_ptr = (char**)arg;
    size_t* count_ptr = (size_t*)*(buf_ptr + 1);  /* Hack to get count */
    
    if (*count_ptr > 0) {
        **buf_ptr = c;
        (*buf_ptr)++;
        (*count_ptr)--;
    }
}

/**
 * @brief Convert number to string in specified base
 */
static int num_to_str(char* buf, size_t bufsize, unsigned long value, 
                      int base, bool uppercase) {
    char digits[] = "0123456789abcdef";
    char digits_upper[] = "0123456789ABCDEF";
    const char* d = uppercase ? digits_upper : digits;
    
    char temp[32];
    int len = 0;
    
    if (value == 0) {
        if (bufsize > 0) {
            buf[0] = '0';
        }
        return 1;
    }
    
    /* Convert to string in reverse */
    while (value > 0 && len < 31) {
        temp[len++] = d[value % base];
        value /= base;
    }
    
    /* Reverse into output buffer */
    int i = 0;
    while (len > 0 && i < (int)(bufsize - 1)) {
        buf[i++] = temp[--len];
    }
    buf[i] = '\0';
    
    return i;
}

/**
 * @brief Internal printf implementation
 */
int vprintf_custom(putc_func_t putc_fn, void* arg, 
                   const char* format, va_list args) {
    int count = 0;
    const char* p = format;
    
    while (*p) {
        if (*p != '%') {
            /* Regular character */
            putc_fn(*p, arg);
            count++;
            p++;
            continue;
        }
        
        p++;  /* Skip '%' */
        
        if (*p == '\0') {
            break;
        }
        
        /* Parse flags (simplified - only handle basic cases) */
        int width = 0;
        int precision = -1;
        int length_mod = 0;  /* 0=none, 1=l, 2=ll, 3=z */
        
        /* Width */
        while (*p >= '0' && *p <= '9') {
            width = width * 10 + (*p - '0');
            p++;
        }
        
        /* Precision */
        if (*p == '.') {
            p++;
            precision = 0;
            while (*p >= '0' && *p <= '9') {
                precision = precision * 10 + (*p - '0');
                p++;
            }
        }
        
        /* Length modifier */
        if (*p == 'l') {
            length_mod = 1;
            p++;
            if (*p == 'l') {
                length_mod = 2;
                p++;
            }
        } else if (*p == 'z') {
            length_mod = 3;
            p++;
        }
        
        /* Conversion specifier */
        switch (*p) {
            case 'd':
            case 'i': {
                long value;
                if (length_mod >= 1) {
                    value = va_arg(args, long);
                } else {
                    value = va_arg(args, int);
                }
                
                bool negative = false;
                unsigned long abs_value;
                
                if (value < 0) {
                    negative = true;
                    abs_value = (unsigned long)(-value);
                } else {
                    abs_value = (unsigned long)value;
                }
                
                char buf[32];
                int len = num_to_str(buf, sizeof(buf), abs_value, 10, false);
                
                /* Print sign first */
                if (negative) {
                    putc_fn('-', arg);
                    count++;
                }
                
                for (int i = 0; i < len; i++) {
                    putc_fn(buf[i], arg);
                }
                count += len;
                break;
            }
            
            case 'u': {
                unsigned long value;
                if (length_mod >= 1) {
                    value = va_arg(args, unsigned long);
                } else {
                    value = va_arg(args, unsigned int);
                }
                
                char buf[32];
                int len = num_to_str(buf, sizeof(buf), value, 10, false);
                
                for (int i = 0; i < len; i++) {
                    putc_fn(buf[i], arg);
                }
                count += len;
                break;
            }
            
            case 'x':
            case 'X': {
                unsigned long value;
                if (length_mod >= 1) {
                    value = va_arg(args, unsigned long);
                } else {
                    value = va_arg(args, unsigned int);
                }
                
                char buf[32];
                int len = num_to_str(buf, sizeof(buf), value, 16, *p == 'X');
                
                for (int i = 0; i < len; i++) {
                    putc_fn(buf[i], arg);
                }
                count += len;
                break;
            }
            
            case 'p': {
                void* ptr = va_arg(args, void*);
                unsigned long value = (unsigned long)ptr;
                
                putc_fn('0', arg);
                putc_fn('x', arg);
                count += 2;
                
                char buf[32];
                int len = num_to_str(buf, sizeof(buf), value, 16, false);
                
                for (int i = 0; i < len; i++) {
                    putc_fn(buf[i], arg);
                }
                count += len;
                break;
            }
            
            case 's': {
                const char* str = va_arg(args, const char*);
                if (str == NULL) {
                    str = "(null)";
                }
                
                int len = 0;
                while (*str) {
                    if (precision >= 0 && len >= precision) {
                        break;
                    }
                    putc_fn(*str++, arg);
                    len++;
                }
                count += len;
                break;
            }
            
            case 'c': {
                char c = (char)va_arg(args, int);
                putc_fn(c, arg);
                count++;
                break;
            }
            
            case '%':
                putc_fn('%', arg);
                count++;
                break;
            
            default:
                /* Unknown specifier - print as-is */
                putc_fn('%', arg);
                putc_fn(*p, arg);
                count += 2;
                break;
        }
        
        p++;
    }
    
    return count;
}

/**
 * @brief Buffer state for snprintf
 */
typedef struct {
    char* buf;
    size_t remaining;
    size_t total;
} snprintf_state_t;

/**
 * @brief Put character helper for snprintf
 */
static void snprintf_putc(char c, void* arg) {
    snprintf_state_t* state = (snprintf_state_t*)arg;
    
    state->total++;
    
    if (state->remaining > 1) {
        *state->buf++ = c;
        state->remaining--;
    }
}

/**
 * @brief Format and print to buffer
 */
int vsnprintf(char* buf, size_t size, const char* format, va_list args) {
    if (buf == NULL || size == 0 || format == NULL) {
        return 0;
    }
    
    snprintf_state_t state = {
        .buf = buf,
        .remaining = size,
        .total = 0
    };
    
    vprintf_custom(snprintf_putc, &state, format, args);
    
    /* Null terminate */
    if (state.remaining > 0) {
        *state.buf = '\0';
    } else if (size > 0) {
        buf[size - 1] = '\0';
    }
    
    return state.total;
}

/**
 * @brief Format and print to buffer (varargs version)
 */
int snprintf(char* buf, size_t size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vsnprintf(buf, size, format, args);
    va_end(args);
    return result;
}
