/* ============================================================================
 * KOS Bootloader - PS/2 Keyboard Driver
 * ============================================================================
 * PS/2 keyboard driver with scancode set 2 support
 * Interrupt 0x21 handler, polling mode for bootloader
 * ============================================================================
 */

#ifndef KOS_KEYBOARD_H
#define KOS_KEYBOARD_H

#include "types.h"

/* ============================================================================
 * PS/2 Scancode Set 2 Definitions
 * ============================================================================
 */

/* Key release prefix in scancode set 2 */
#define SCAN_RELEASE    0x80

/* Modifier keys */
#define SCAN_LCTRL      0x14
#define SCAN_RCTRL      0x58
#define SCAN_LSHIFT     0x12
#define SCAN_RSHIFT     0x59
#define SCAN_LALT       0x11
#define SCAN_RALT       0xE038
#define SCAN_CAPSLOCK   0x58

/* Special keys */
#define SCAN_ENTER      0x1C
#define SCAN_ESCAPE     0x01
#define SCAN_BACKSPACE  0x0E
#define SCAN_TAB        0x0F
#define SCAN_SPACE      0x39

/* Arrow keys */
#define SCAN_UP         0x48
#define SCAN_DOWN       0x50
#define SCAN_LEFT       0x4B
#define SCAN_RIGHT      0x4D

/* Function keys */
#define SCAN_F1         0x3B
#define SCAN_F2         0x3C
#define SCAN_F3         0x3D
#define SCAN_F4         0x3E
#define SCAN_F5         0x3F
#define SCAN_F6         0x40
#define SCAN_F7         0x41
#define SCAN_F8         0x42
#define SCAN_F9         0x43
#define SCAN_F10        0x44
#define SCAN_F11        0x57
#define SCAN_F12        0x58

/* Alphanumeric keys */
#define SCAN_A          0x1E
#define SCAN_B          0x30
#define SCAN_C          0x2E
#define SCAN_D          0x20
#define SCAN_E          0x12
#define SCAN_F          0x21
#define SCAN_G          0x22
#define SCAN_H          0x23
#define SCAN_I          0x17
#define SCAN_J          0x24
#define SCAN_K          0x25
#define SCAN_L          0x26
#define SCAN_M          0x32
#define SCAN_N          0x31
#define SCAN_O          0x18
#define SCAN_P          0x19
#define SCAN_Q          0x10
#define SCAN_R          0x13
#define SCAN_S          0x1F
#define SCAN_T          0x14
#define SCAN_U          0x16
#define SCAN_V          0x2F
#define SCAN_W          0x11
#define SCAN_X          0x2D
#define SCAN_Y          0x15
#define SCAN_Z          0x2C

/* Number keys */
#define SCAN_0          0x0B
#define SCAN_1          0x02
#define SCAN_2          0x03
#define SCAN_3          0x04
#define SCAN_4          0x05
#define SCAN_5          0x06
#define SCAN_6          0x07
#define SCAN_7          0x08
#define SCAN_8          0x09
#define SCAN_9          0x0A

/* ============================================================================
 * Keyboard Buffer Configuration
 * ============================================================================
 */

#define KEYBOARD_BUFFER_SIZE  256

/* ============================================================================
 * Function Prototypes
 * ============================================================================
 */

/**
 * @brief Initialize PS/2 keyboard
 * Sets up keyboard controller and enables interrupts
 */
void keyboard_init(void);

/**
 * @brief Check if a key is available in buffer
 * @return true if key available, false otherwise
 */
bool keyboard_check_key(void);

/**
 * @brief Get next key from buffer (blocking)
 * @return Scancode of key pressed
 */
uint8_t keyboard_get_key(void);

/**
 * @brief Get next key from buffer (non-blocking)
 * @param key Pointer to store scancode
 * @return true if key was read, false if buffer empty
 */
bool keyboard_try_get_key(uint8_t* key);

/**
 * @brief Clear keyboard buffer
 */
void keyboard_clear_buffer(void);

/**
 * @brief Wait for keyboard controller to be ready
 */
void keyboard_wait_ready(void);

/**
 * @brief Send command to keyboard controller
 * @param cmd Command byte to send
 * @return true if command acknowledged
 */
bool keyboard_send_cmd(uint8_t cmd);

/**
 * @brief Read data from keyboard
 * @return Data byte read
 */
uint8_t keyboard_read_data(void);

/**
 * @brief Convert scancode to ASCII character
 * @param scancode Scancode from keyboard
 * @param shift Shift state (true if shift pressed)
 * @return ASCII character or 0 if not printable
 */
char scancode_to_ascii(uint8_t scancode, bool shift);

/**
 * @brief Get current modifier key state
 * @return Bitmask of modifier states
 */
uint8_t keyboard_get_modifiers(void);

#endif /* KOS_KEYBOARD_H */
