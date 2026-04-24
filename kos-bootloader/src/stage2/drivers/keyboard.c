/* ============================================================================
 * KOS Bootloader - PS/2 Keyboard Driver Implementation
 * ============================================================================
 * PS/2 keyboard driver with scancode set 2 support
 * Polling mode for bootloader environment
 * ============================================================================
 */

#include "keyboard.h"
#include "io.h"
#include "string.h"

/* ============================================================================
 * PS/2 Controller Ports and Commands
 * ============================================================================
 */

#define KBD_DATA_PORT       0x60
#define KBD_STATUS_PORT     0x64
#define KBD_CMD_PORT        0x64

/* Status register bits */
#define KBD_STATUS_OUTPUT   0x01    /* Output buffer full */
#define KBD_STATUS_INPUT    0x02    /* Input buffer full */
#define KBD_STATUS_SYSTEM   0x04    /* System flag */
#define KBD_STATUS_COMMAND  0x08    /* Command/data flag */
#define KBD_STATUS_TIMEOUT  0x40    /* Timeout error */
#define KBD_STATUS_PARITY   0x80    /* Parity error */

/* Keyboard controller commands */
#define KBD_CMD_READ_CONFIG     0x20
#define KBD_CMD_WRITE_CONFIG    0x60
#define KBD_CMD_DISABLE_KBD     0xAD
#define KBD_CMD_ENABLE_KBD      0xAE
#define KBD_CMD_TEST_KBD        0xAB
#define KBD_CMD_SEND_TO_KBD     0xD2

/* Keyboard commands */
#define KBD_SET_SCANCODE    0xF0    /* Set scancode set */
#define KBD_SET_LEDS        0xED    /* Set LEDs */
#define KBD_ECHO            0xEE    /* Echo */
#define KBD_ACK             0xFA    /* Acknowledge */
#define KBD_RESEND          0xFE    /* Resend */

/* ============================================================================
 * Keyboard State
 * ============================================================================
 */

static uint8_t keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static volatile size_t buffer_head = 0;
static volatile size_t buffer_tail = 0;

static bool shift_pressed = false;
static bool ctrl_pressed = false;
static bool alt_pressed = false;
static bool caps_lock = false;

/* ============================================================================
 * Scancode to ASCII conversion table (US QWERTY layout)
 * ============================================================================
 */

static const char scan_ascii_table[128] = {
    [0x01] = 0,       /* ESC */
    [0x02] = '1',
    [0x03] = '2',
    [0x04] = '3',
    [0x05] = '4',
    [0x06] = '5',
    [0x07] = '6',
    [0x08] = '7',
    [0x09] = '8',
    [0x0A] = '9',
    [0x0B] = '0',
    [0x0C] = '-',
    [0x0D] = '=',
    [0x0E] = '\b',    /* Backspace */
    [0x0F] = '\t',    /* Tab */
    [0x10] = 'q',
    [0x11] = 'w',
    [0x12] = 'e',
    [0x13] = 'r',
    [0x14] = 't',
    [0x15] = 'y',
    [0x16] = 'u',
    [0x17] = 'i',
    [0x18] = 'o',
    [0x19] = 'p',
    [0x1A] = '[',
    [0x1B] = ']',
    [0x1C] = '\n',    /* Enter */
    [0x1D] = 0,       /* LCtrl */
    [0x1E] = 'a',
    [0x1F] = 's',
    [0x20] = 'd',
    [0x21] = 'f',
    [0x22] = 'g',
    [0x23] = 'h',
    [0x24] = 'j',
    [0x25] = 'k',
    [0x26] = 'l',
    [0x27] = ';',
    [0x28] = '\'',
    [0x29] = '`',
    [0x2A] = 0,       /* LShift */
    [0x2B] = '\\',
    [0x2C] = 'z',
    [0x2D] = 'x',
    [0x2E] = 'c',
    [0x2F] = 'v',
    [0x30] = 'b',
    [0x31] = 'n',
    [0x32] = 'm',
    [0x33] = ',',
    [0x34] = '.',
    [0x35] = '/',
    [0x36] = 0,       /* RShift */
    [0x37] = '*',     /* Keypad */
    [0x38] = 0,       /* LAlt */
    [0x39] = ' ',     /* Space */
    [0x3A] = 0,       /* CapsLock */
};

/* Shifted characters */
static const char scan_ascii_shifted[128] = {
    [0x02] = '!',
    [0x03] = '@',
    [0x04] = '#',
    [0x05] = '$',
    [0x06] = '%',
    [0x07] = '^',
    [0x08] = '&',
    [0x09] = '*',
    [0x0A] = '(',
    [0x0B] = ')',
    [0x0C] = '_',
    [0x0D] = '+',
    [0x10] = 'Q',
    [0x11] = 'W',
    [0x12] = 'E',
    [0x13] = 'R',
    [0x14] = 'T',
    [0x15] = 'Y',
    [0x16] = 'U',
    [0x17] = 'I',
    [0x18] = 'O',
    [0x19] = 'P',
    [0x1A] = '{',
    [0x1B] = '}',
    [0x1E] = 'A',
    [0x1F] = 'S',
    [0x20] = 'D',
    [0x21] = 'F',
    [0x22] = 'G',
    [0x23] = 'H',
    [0x24] = 'J',
    [0x25] = 'K',
    [0x26] = 'L',
    [0x27] = ':',
    [0x28] = '"',
    [0x29] = '~',
    [0x2B] = '|',
    [0x2C] = 'Z',
    [0x2D] = 'X',
    [0x2E] = 'C',
    [0x2F] = 'V',
    [0x30] = 'B',
    [0x31] = 'N',
    [0x32] = 'M',
    [0x33] = '<',
    [0x34] = '>',
    [0x35] = '?',
    [0x37] = '*',
};

/* ============================================================================
 * Internal Helper Functions
 * ============================================================================
 */

/**
 * @brief Add scancode to buffer
 */
static void buffer_push(uint8_t scancode) {
    size_t next_head = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
    
    /* Don't overwrite if buffer is full */
    if (next_head != buffer_tail) {
        keyboard_buffer[buffer_head] = scancode;
        buffer_head = next_head;
    }
}

/**
 * @brief Remove scancode from buffer
 */
static bool buffer_pop(uint8_t* scancode) {
    if (buffer_head == buffer_tail) {
        return false;  /* Buffer empty */
    }
    
    *scancode = keyboard_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return true;
}

/**
 * @brief Check if buffer is empty
 */
static bool buffer_is_empty(void) {
    return buffer_head == buffer_tail;
}

/* ============================================================================
 * Public API Implementation
 * ============================================================================
 */

void keyboard_init(void) {
    /* Clear buffer */
    memset(keyboard_buffer, 0, sizeof(keyboard_buffer));
    buffer_head = 0;
    buffer_tail = 0;
    
    /* Reset modifier state */
    shift_pressed = false;
    ctrl_pressed = false;
    alt_pressed = false;
    caps_lock = false;
    
    /* Wait for keyboard controller to be ready */
    keyboard_wait_ready();
    
    /* Enable keyboard */
    outb(KBD_CMD_PORT, KBD_CMD_ENABLE_KBD);
    
    /* Small delay */
    for (volatile int i = 0; i < 10000; i++);
    
    /* Flush any pending data */
    while (inb(KBD_STATUS_PORT) & KBD_STATUS_OUTPUT) {
        inb(KBD_DATA_PORT);
    }
}

bool keyboard_check_key(void) {
    /* First check our buffer */
    if (!buffer_is_empty()) {
        return true;
    }
    
    /* Then check hardware */
    return (inb(KBD_STATUS_PORT) & KBD_STATUS_OUTPUT) != 0;
}

uint8_t keyboard_get_key(void) {
    uint8_t key;
    
    /* Block until a key is available */
    while (!keyboard_try_get_key(&key)) {
        /* Wait */
    }
    
    return key;
}

bool keyboard_try_get_key(uint8_t* key) {
    /* Try to get from buffer first */
    if (buffer_pop(key)) {
        return true;
    }
    
    /* Check if hardware has data */
    if (!(inb(KBD_STATUS_PORT) & KBD_STATUS_OUTPUT)) {
        return false;
    }
    
    /* Read scancode from hardware */
    *key = inb(KBD_DATA_PORT);
    
    /* Process the scancode */
    uint8_t scan = *key & 0x7F;  /* Remove release bit */
    bool released = (*key & SCAN_RELEASE) != 0;
    
    /* Update modifier state */
    switch (scan) {
        case SCAN_LSHIFT:
        case SCAN_RSHIFT:
            shift_pressed = !released;
            break;
        case SCAN_LCTRL:
        case SCAN_RCTRL:
            ctrl_pressed = !released;
            break;
        case SCAN_LALT:
            alt_pressed = !released;
            break;
        default:
            /* Store non-modifier keys in buffer */
            if (!released) {
                buffer_push(*key);
            }
            break;
    }
    
    return true;
}

void keyboard_clear_buffer(void) {
    buffer_head = 0;
    buffer_tail = 0;
    
    /* Also flush hardware buffer */
    while (inb(KBD_STATUS_PORT) & KBD_STATUS_OUTPUT) {
        inb(KBD_DATA_PORT);
    }
}

void keyboard_wait_ready(void) {
    /* Wait for input buffer to be empty */
    for (int timeout = 100000; timeout > 0; timeout--) {
        if (!(inb(KBD_STATUS_PORT) & KBD_STATUS_INPUT)) {
            return;
        }
        io_wait();
    }
}

bool keyboard_send_cmd(uint8_t cmd) {
    keyboard_wait_ready();
    
    outb(KBD_DATA_PORT, cmd);
    
    /* Wait for acknowledgment */
    for (int timeout = 10000; timeout > 0; timeout--) {
        if (inb(KBD_STATUS_PORT) & KBD_STATUS_OUTPUT) {
            uint8_t response = inb(KBD_DATA_PORT);
            if (response == KBD_ACK) {
                return true;
            } else if (response == KBD_RESEND) {
                /* Retry */
                return keyboard_send_cmd(cmd);
            }
        }
        io_wait();
    }
    
    return false;
}

uint8_t keyboard_read_data(void) {
    /* Wait for output buffer to be full */
    while (!(inb(KBD_STATUS_PORT) & KBD_STATUS_OUTPUT)) {
        io_wait();
    }
    
    return inb(KBD_DATA_PORT);
}

char scancode_to_ascii(uint8_t scancode, bool shift) {
    /* Remove release bit */
    uint8_t scan = scancode & 0x7F;
    
    if (scan >= 128) {
        return 0;
    }
    
    if (shift || caps_lock) {
        char c = scan_ascii_shifted[scan];
        if (c != 0) {
            return c;
        }
    }
    
    return scan_ascii_table[scan];
}

uint8_t keyboard_get_modifiers(void) {
    uint8_t modifiers = 0;
    
    if (shift_pressed) {
        modifiers |= 0x01;
    }
    if (ctrl_pressed) {
        modifiers |= 0x02;
    }
    if (alt_pressed) {
        modifiers |= 0x04;
    }
    if (caps_lock) {
        modifiers |= 0x08;
    }
    
    return modifiers;
}
