/* ============================================================================
 * KOS Bootloader - String Library Implementation
 * ============================================================================
 * Minimal string manipulation functions (no libc dependency)
 * ============================================================================
 */

#include "string.h"

/**
 * @brief Copy memory area
 */
void* memcpy(void* dest, const void* src, size_t n) {
    if (dest == NULL || src == NULL) {
        return NULL;
    }
    
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    
    while (n--) {
        *d++ = *s++;
    }
    
    return dest;
}

/**
 * @brief Move memory area (handles overlap)
 */
void* memmove(void* dest, const void* src, size_t n) {
    if (dest == NULL || src == NULL) {
        return NULL;
    }
    
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    
    if (d < s) {
        /* Copy forward */
        while (n--) {
            *d++ = *s++;
        }
    } else if (d > s) {
        /* Copy backward to handle overlap */
        d += n;
        s += n;
        while (n--) {
            *--d = *--s;
        }
    }
    /* If d == s, nothing to do */
    
    return dest;
}

/**
 * @brief Set memory area to value
 */
void* memset(void* s, int c, size_t n) {
    if (s == NULL) {
        return NULL;
    }
    
    uint8_t* p = (uint8_t*)s;
    uint8_t value = (uint8_t)c;
    
    while (n--) {
        *p++ = value;
    }
    
    return s;
}

/**
 * @brief Compare memory areas
 */
int memcmp(const void* s1, const void* s2, size_t n) {
    if (s1 == NULL || s2 == NULL) {
        return 0;  /* Treat NULL as equal for safety */
    }
    
    const uint8_t* p1 = (const uint8_t*)s1;
    const uint8_t* p2 = (const uint8_t*)s2;
    
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    
    return 0;
}

/**
 * @brief Get string length
 */
size_t strlen(const char* s) {
    if (s == NULL) {
        return 0;
    }
    
    size_t len = 0;
    while (*s++) {
        len++;
    }
    
    return len;
}

/**
 * @brief Copy string
 */
char* strcpy(char* dest, const char* src) {
    if (dest == NULL || src == NULL) {
        return NULL;
    }
    
    char* d = dest;
    while ((*d++ = *src++) != '\0') {
        /* empty */
    }
    
    return dest;
}

/**
 * @brief Copy string with size limit
 */
char* strncpy(char* dest, const char* src, size_t n) {
    if (dest == NULL || src == NULL) {
        return NULL;
    }
    
    char* d = dest;
    size_t i;
    
    for (i = 0; i < n && *src != '\0'; i++) {
        *d++ = *src++;
    }
    
    /* Pad with null bytes if needed */
    while (i < n) {
        *d++ = '\0';
        i++;
    }
    
    return dest;
}

/**
 * @brief Concatenate strings
 */
char* strcat(char* dest, const char* src) {
    if (dest == NULL || src == NULL) {
        return NULL;
    }
    
    char* d = dest;
    
    /* Find end of destination */
    while (*d) {
        d++;
    }
    
    /* Copy source to end */
    while ((*d++ = *src++) != '\0') {
        /* empty */
    }
    
    return dest;
}

/**
 * @brief Compare strings
 */
int strcmp(const char* s1, const char* s2) {
    if (s1 == NULL || s2 == NULL) {
        return 0;  /* Treat NULL as equal for safety */
    }
    
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

/**
 * @brief Compare strings with length limit
 */
int strncmp(const char* s1, const char* s2, size_t n) {
    if (s1 == NULL || s2 == NULL) {
        return 0;
    }
    
    if (n == 0) {
        return 0;
    }
    
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    
    if (n == 0) {
        return 0;
    }
    
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

/**
 * @brief Find character in string
 */
char* strchr(const char* s, int c) {
    if (s == NULL) {
        return NULL;
    }
    
    while (*s) {
        if (*s == (char)c) {
            return (char*)s;
        }
        s++;
    }
    
    /* Check for null terminator */
    if ((char)c == '\0') {
        return (char*)s;
    }
    
    return NULL;
}

/**
 * @brief Find substring in string
 */
char* strstr(const char* haystack, const char* needle) {
    if (haystack == NULL || needle == NULL) {
        return NULL;
    }
    
    /* Empty needle matches at start */
    if (*needle == '\0') {
        return (char*)haystack;
    }
    
    size_t needle_len = strlen(needle);
    
    while (*haystack) {
        if (strncmp(haystack, needle, needle_len) == 0) {
            return (char*)haystack;
        }
        haystack++;
    }
    
    return NULL;
}

/**
 * @brief Duplicate string
 */
char* strdup(const char* s) {
    if (s == NULL) {
        return NULL;
    }
    
    size_t len = strlen(s) + 1;  /* +1 for null terminator */
    char* dup = (char*)s;  /* Note: In bootloader, we don't have malloc */
    
    /* For bootloader use, this is a simplified version */
    /* In real code, you'd need to allocate memory somehow */
    if (dup != NULL) {
        memcpy(dup, s, len);
    }
    
    return dup;
}

/**
 * @brief Convert string to long integer
 */
long strtol(const char* nptr, char** endptr, int base) {
    const char* s = nptr;
    long result = 0;
    bool negative = false;
    
    if (s == NULL) {
        if (endptr) *endptr = NULL;
        return 0;
    }
    
    /* Skip whitespace */
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') {
        s++;
    }
    
    /* Handle sign */
    if (*s == '-') {
        negative = true;
        s++;
    } else if (*s == '+') {
        s++;
    }
    
    /* Auto-detect base */
    if (base == 0) {
        if (*s == '0') {
            if (s[1] == 'x' || s[1] == 'X') {
                base = 16;
                s += 2;
            } else {
                base = 8;
                s++;
            }
        } else {
            base = 10;
        }
    } else if (base == 16 && *s == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
    }
    
    /* Convert digits */
    while (*s) {
        int digit;
        
        if (*s >= '0' && *s <= '9') {
            digit = *s - '0';
        } else if (*s >= 'a' && *s <= 'z') {
            digit = *s - 'a' + 10;
        } else if (*s >= 'A' && *s <= 'Z') {
            digit = *s - 'A' + 10;
        } else {
            break;
        }
        
        if (digit >= base) {
            break;
        }
        
        result = result * base + digit;
        s++;
    }
    
    if (endptr) {
        *endptr = (char*)s;
    }
    
    return negative ? -result : result;
}

/**
 * @brief Convert string to unsigned long
 */
unsigned long strtoul(const char* nptr, char** endptr, int base) {
    const char* s = nptr;
    unsigned long result = 0;
    
    if (s == NULL) {
        if (endptr) *endptr = NULL;
        return 0;
    }
    
    /* Skip whitespace */
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') {
        s++;
    }
    
    /* Handle optional sign (ignore for unsigned) */
    if (*s == '-' || *s == '+') {
        s++;
    }
    
    /* Auto-detect base */
    if (base == 0) {
        if (*s == '0') {
            if (s[1] == 'x' || s[1] == 'X') {
                base = 16;
                s += 2;
            } else {
                base = 8;
                s++;
            }
        } else {
            base = 10;
        }
    } else if (base == 16 && *s == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
    }
    
    /* Convert digits */
    while (*s) {
        int digit;
        
        if (*s >= '0' && *s <= '9') {
            digit = *s - '0';
        } else if (*s >= 'a' && *s <= 'z') {
            digit = *s - 'a' + 10;
        } else if (*s >= 'A' && *s <= 'Z') {
            digit = *s - 'A' + 10;
        } else {
            break;
        }
        
        if (digit >= base) {
            break;
        }
        
        result = result * base + digit;
        s++;
    }
    
    if (endptr) {
        *endptr = (char*)s;
    }
    
    return result;
}
