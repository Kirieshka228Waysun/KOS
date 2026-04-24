/* ============================================================================
 * KOS Bootloader - I/O Operations Header
 * ============================================================================
 * Low-level port I/O and CPU control functions
 * ============================================================================
 */

#ifndef KOS_IO_H
#define KOS_IO_H

#include "types.h"

/**
 * @brief Read a byte from an I/O port
 * @param port Port number (0-65535)
 * @return Byte read from port
 */
static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    asm volatile("inb %w1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/**
 * @brief Read a word (16-bit) from an I/O port
 * @param port Port number
 * @return Word read from port
 */
static inline uint16_t inw(uint16_t port) {
    uint16_t value;
    asm volatile("inw %w1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/**
 * @brief Read a double-word (32-bit) from an I/O port
 * @param port Port number
 * @return Double-word read from port
 */
static inline uint32_t inl(uint16_t port) {
    uint32_t value;
    asm volatile("inl %w1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/**
 * @brief Write a byte to an I/O port
 * @param port Port number
 * @param value Byte to write
 */
static inline void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %w1" : : "a"(value), "Nd"(port));
}

/**
 * @brief Write a word (16-bit) to an I/O port
 * @param port Port number
 * @param value Word to write
 */
static inline void outw(uint16_t port, uint16_t value) {
    asm volatile("outw %0, %w1" : : "a"(value), "Nd"(port));
}

/**
 * @brief Write a double-word (32-bit) to an I/O port
 * @param port Port number
 * @param value Double-word to write
 */
static inline void outl(uint16_t port, uint32_t value) {
    asm volatile("outl %0, %w1" : : "a"(value), "Nd"(port));
}

/**
 * @brief Disable interrupts (CLI)
 */
static inline void cli(void) {
    asm volatile("cli" ::: "memory");
}

/**
 * @brief Enable interrupts (STI)
 */
static inline void sti(void) {
    asm volatile("sti" ::: "memory");
}

/**
 * @brief Halt CPU until next interrupt (HLT)
 */
static inline void hlt(void) {
    asm volatile("hlt" ::: "memory");
}

/**
 * @brief No operation (NOP)
 */
static inline void nop(void) {
    asm volatile("nop" ::: "memory");
}

/**
 * @brief Read I/O port with delay (for slow devices)
 * @param port Port number
 * @return Byte read from port
 */
static inline uint8_t inb_p(uint16_t port) {
    uint8_t value;
    asm volatile(
        "inb %w1, %0\n\t"
        "jmp 1f\n\t"
        "1:\n\t"
        "jmp 1f\n\t"
        "1:\n\t"
        : "=a"(value) : "Nd"(port));
    return value;
}

/**
 * @brief Write I/O port with delay (for slow devices)
 * @param port Port number
 * @param value Byte to write
 */
static inline void outb_p(uint16_t port, uint8_t value) {
    asm volatile(
        "outb %0, %w1\n\t"
        "jmp 1f\n\t"
        "1:\n\t"
        "jmp 1f\n\t"
        "1:\n\t"
        : : "a"(value), "Nd"(port));
}

/**
 * @brief Wait for approximately 1 microsecond
 * Uses port 0x80 POST code port for timing
 */
static inline void io_wait(void) {
    /* Port 0x80 is used for POST codes, reading it causes a small delay */
    asm volatile(
        "outb %%al, $0x80"
        :
        : "a"(0));
}

/**
 * @brief Read CR0 control register
 * @return CR0 value
 */
static inline uint32_t read_cr0(void) {
    uint32_t value;
    asm volatile("movl %%cr0, %0" : "=r"(value));
    return value;
}

/**
 * @brief Write CR0 control register
 * @param value New CR0 value
 */
static inline void write_cr0(uint32_t value) {
    asm volatile("movl %0, %%cr0" : : "r"(value));
}

/**
 * @brief Read CR2 control register (page fault linear address)
 * @return CR2 value
 */
static inline uint32_t read_cr2(void) {
    uint32_t value;
    asm volatile("movl %%cr2, %0" : "=r"(value));
    return value;
}

/**
 * @brief Read CR3 control register (page directory base)
 * @return CR3 value
 */
static inline uint32_t read_cr3(void) {
    uint32_t value;
    asm volatile("movl %%cr3, %0" : "=r"(value));
    return value;
}

/**
 * @brief Read CR4 control register
 * @return CR4 value
 */
static inline uint32_t read_cr4(void) {
    uint32_t value;
    asm volatile("movl %%cr4, %0" : "=r"(value));
    return value;
}

/**
 * @brief Write CR4 control register
 * @param value New CR4 value
 */
static inline void write_cr4(uint32_t value) {
    asm volatile("movl %0, %%cr4" : : "r"(value));
}

/**
 * @brief Read EFLAGS register
 * @return EFLAGS value
 */
static inline uint32_t read_eflags(void) {
    uint32_t eflags;
    asm volatile("pushfl; popl %0" : "=rm"(eflags));
    return eflags;
}

/**
 * @brief Check if interrupts are enabled
 * @return true if interrupts enabled, false otherwise
 */
static inline bool interrupts_enabled(void) {
    return (read_eflags() & 0x200) != 0;
}

/**
 * @brief Save current interrupt state and disable interrupts
 * @return Previous interrupt state (true if was enabled)
 */
static inline bool save_and_disable_interrupts(void) {
    bool was_enabled = interrupts_enabled();
    cli();
    return was_enabled;
}

/**
 * @brief Restore interrupt state
 * @param enable true to enable interrupts, false to keep disabled
 */
static inline void restore_interrupts(bool enable) {
    if (enable) {
        sti();
    }
}

#endif /* KOS_IO_H */
