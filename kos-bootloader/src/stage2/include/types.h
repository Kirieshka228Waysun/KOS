/* ============================================================================
 * KOS Bootloader - Type Definitions
 * ============================================================================
 * Standard integer types and basic definitions for freestanding C code
 * No libc dependency
 * ============================================================================
 */

#ifndef KOS_TYPES_H
#define KOS_TYPES_H

/* NULL pointer definition */
#define NULL ((void*)0)

/* Boolean type */
typedef enum {
    false = 0,
    true = 1
} bool;

/* Fixed-width integer types (i686, 32-bit) */
typedef unsigned char       uint8_t;
typedef signed char         int8_t;
typedef unsigned short      uint16_t;
typedef signed short        int16_t;
typedef unsigned int        uint32_t;
typedef signed int          int32_t;
typedef unsigned long long  uint64_t;
typedef signed long long    int64_t;

/* Pointer-sized integer types */
typedef unsigned int        uintptr_t;
typedef signed int          intptr_t;

/* Size type */
typedef uint32_t            size_t;
typedef int32_t             ssize_t;

/* Physical and virtual address types */
typedef uint32_t            phys_addr_t;
typedef uint32_t            virt_addr_t;

/* Volatile qualifier for memory-mapped I/O */
#define volatile_register   volatile

/* Memory barriers - use __asm__ directly to avoid macro issues */
#define memory_barrier()    __asm__ volatile("" ::: "memory")
#define read_barrier()      __asm__ volatile("" ::: "memory")
#define write_barrier()     __asm__ volatile("" : : : "memory")

/* Container of macro - get struct from member pointer */
#define container_of(ptr, type, member) \
    ((type*)((char*)(ptr) - offsetof(type, member)))

/* Offset of macro - get offset of member in struct */
#define offsetof(type, member) ((size_t)&(((type*)0)->member))

/* Min/Max macros */
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

/* Array size macro */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* Packed structure attribute */
#define PACKED __attribute__((packed))

/* Aligned attribute */
#define ALIGNED(x) __attribute__((aligned(x)))

/* Noreturn attribute */
#define NORETURN __attribute__((noreturn))

/* Always inline attribute */
#define ALWAYS_INLINE __attribute__((always_inline))

/* Unused parameter attribute */
#define UNUSED __attribute__((unused))

/* Format attribute for printf-like functions */
#define PRINTF_FORMAT(fmt_idx, first_arg) \
    __attribute__((format(printf, fmt_idx, first_arg)))

/* Section attribute */
#define SECTION(x) __attribute__((section(x)))

/* Weak symbol attribute */
#define WEAK __attribute__((weak))

/* Likely/unlikely branch prediction hints */
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#endif /* KOS_TYPES_H */
