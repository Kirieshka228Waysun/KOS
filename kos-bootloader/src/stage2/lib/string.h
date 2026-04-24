/* ============================================================================
 * KOS Bootloader - String Library
 * ============================================================================
 * Minimal string manipulation functions (no libc dependency)
 * ============================================================================
 */

#ifndef KOS_STRING_H
#define KOS_STRING_H

#include "types.h"

/**
 * @brief Copy memory area
 * @param dest Destination pointer
 * @param src Source pointer
 * @param n Number of bytes to copy
 * @return Pointer to destination
 */
void* memcpy(void* dest, const void* src, size_t n);

/**
 * @brief Move memory area (handles overlap)
 * @param dest Destination pointer
 * @param src Source pointer
 * @param n Number of bytes to move
 * @return Pointer to destination
 */
void* memmove(void* dest, const void* src, size_t n);

/**
 * @brief Set memory area to value
 * @param s Memory area to set
 * @param c Value to set (converted to unsigned char)
 * @param n Number of bytes to set
 * @return Pointer to memory area
 */
void* memset(void* s, int c, size_t n);

/**
 * @brief Compare memory areas
 * @param s1 First memory area
 * @param s2 Second memory area
 * @param n Number of bytes to compare
 * @return 0 if equal, <0 if s1<s2, >0 if s1>s2
 */
int memcmp(const void* s1, const void* s2, size_t n);

/**
 * @brief Get string length
 * @param s Null-terminated string
 * @return Length of string (not including null terminator)
 */
size_t strlen(const char* s);

/**
 * @brief Copy string
 * @param dest Destination buffer
 * @param src Source string
 * @return Pointer to destination
 */
char* strcpy(char* dest, const char* src);

/**
 * @brief Copy string with size limit
 * @param dest Destination buffer
 * @param src Source string
 * @param n Maximum number of bytes to copy
 * @return Pointer to destination
 */
char* strncpy(char* dest, const char* src, size_t n);

/**
 * @brief Concatenate strings
 * @param dest Destination buffer (must have space)
 * @param src Source string to append
 * @return Pointer to destination
 */
char* strcat(char* dest, const char* src);

/**
 * @brief Compare strings
 * @param s1 First string
 * @param s2 Second string
 * @return 0 if equal, <0 if s1<s2, >0 if s1>s2
 */
int strcmp(const char* s1, const char* s2);

/**
 * @brief Compare strings with length limit
 * @param s1 First string
 * @param s2 Second string
 * @param n Maximum characters to compare
 * @return 0 if equal (up to n chars), <0 if s1<s2, >0 if s1>s2
 */
int strncmp(const char* s1, const char* s2, size_t n);

/**
 * @brief Find character in string
 * @param s String to search
 * @param c Character to find
 * @return Pointer to first occurrence, or NULL if not found
 */
char* strchr(const char* s, int c);

/**
 * @brief Find substring in string
 * @param haystack String to search in
 * @param needle Substring to find
 * @return Pointer to first occurrence, or NULL if not found
 */
char* strstr(const char* haystack, const char* needle);

/**
 * @brief Duplicate string (caller must free)
 * @param s String to duplicate
 * @return Pointer to new string, or NULL on failure
 */
char* strdup(const char* s);

/**
 * @brief Convert string to long integer
 * @param nptr String to convert
 * @param endptr If non-NULL, stores pointer to first invalid char
 * @param base Number base (2-36, or 0 for auto-detect)
 * @return Converted value
 */
long strtol(const char* nptr, char** endptr, int base);

/**
 * @brief Convert string to unsigned long
 * @param nptr String to convert
 * @param endptr If non-NULL, stores pointer to first invalid char
 * @param base Number base (2-36, or 0 for auto-detect)
 * @return Converted value
 */
unsigned long strtoul(const char* nptr, char** endptr, int base);

#endif /* KOS_STRING_H */
