/* ============================================================================
 * KOS Bootloader - Serial Driver Implementation (COM1)
 * ============================================================================
 * UART 16550A serial driver for COM1
 * 9600 baud, 8N1, polling mode
 * ============================================================================
 */

#include "serial.h"
#include "io.h"

/* ============================================================================
 * Static Variables
 * ============================================================================
 */

static bool serial_initialized = false;
static uint16_t serial_base = SERIAL_COM1_BASE;

/* ============================================================================
 * Internal Helper Functions
 * ============================================================================
 */

/**
 * @brief Check if UART chip exists at given port
 */
static bool serial_check_uart(void) {
    /* Save original values */
    uint8_t ier_orig = inb(serial_base + UART_IER);
    
    /* Test read/write of IER register */
    outb(serial_base + UART_IER, 0x47);
    uint8_t test = inb(serial_base + UART_IER);
    outb(serial_base + UART_IER, ier_orig);
    
    /* If bits 4-6 are preserved, UART exists */
    return (test & 0x70) == 0x40;
}

/* ============================================================================
 * Public API Implementation
 * ============================================================================
 */

void serial_init(void) {
    /* Disable interrupts */
    outb(serial_base + UART_IER, 0x00);
    
    /* Enable DLAB (set baud rate divisor) */
    outb(serial_base + UART_LCR, UART_LCR_DLAB);
    
    /* Set divisor for 9600 baud (115200 / 9600 = 12) */
    uint16_t divisor = 115200 / SERIAL_BAUD_RATE;
    outb(serial_base + UART_DLL, divisor & 0xFF);        /* Low byte */
    outb(serial_base + UART_DLM, (divisor >> 8) & 0xFF); /* High byte */
    
    /* Clear DLAB, set 8N1 */
    outb(serial_base + UART_LCR, 
         UART_LCR_WLEN_8);  /* 8 bits, no parity, 1 stop bit */
    
    /* Enable FIFO (if 16550) */
    outb(serial_base + UART_FCR, 0x07);  /* Enable and clear FIFOs */
    
    /* Set modem control lines (DTR and RTS) */
    outb(serial_base + UART_MCR, 
         UART_MCR_DTR | UART_MCR_RTS | UART_MCR_AUX2);
    
    /* Clear any pending data */
    serial_clear_rx_buffer();
    
    serial_initialized = true;
}

bool serial_is_initialized(void) {
    return serial_initialized;
}

void serial_putc(char c) {
    if (!serial_initialized) {
        return;
    }
    
    /* Wait until transmitter holding register is empty */
    while (!(inb(serial_base + UART_LSR) & UART_LSR_THRE)) {
        io_wait();
    }
    
    /* Send character */
    outb(serial_base + UART_THR, c);
    
    /* Handle newline */
    if (c == '\n') {
        serial_putc('\r');
    }
}

void serial_puts(const char* str) {
    if (str == NULL) {
        return;
    }
    
    while (*str) {
        serial_putc(*str);
        str++;
    }
}

int serial_getc(void) {
    if (!serial_initialized) {
        return -1;
    }
    
    /* Wait until data is ready */
    while (!(inb(serial_base + UART_LSR) & UART_LSR_DR)) {
        io_wait();
    }
    
    /* Read character */
    return inb(serial_base + UART_RBR);
}

bool serial_data_available(void) {
    if (!serial_initialized) {
        return false;
    }
    
    return (inb(serial_base + UART_LSR) & UART_LSR_DR) != 0;
}

bool serial_try_getc(char* c) {
    if (!serial_initialized || c == NULL) {
        return false;
    }
    
    if (!(inb(serial_base + UART_LSR) & UART_LSR_DR)) {
        return false;
    }
    
    *c = inb(serial_base + UART_RBR);
    return true;
}

void serial_flush(void) {
    if (!serial_initialized) {
        return;
    }
    
    /* Wait until transmitter is completely empty */
    while (!(inb(serial_base + UART_LSR) & UART_LSR_TEMT)) {
        io_wait();
    }
}

void serial_clear_rx_buffer(void) {
    if (!serial_initialized) {
        return;
    }
    
    /* Read until no more data */
    while (inb(serial_base + UART_LSR) & UART_LSR_DR) {
        inb(serial_base + UART_RBR);
        io_wait();
    }
}

void serial_puthex(uint32_t value, int width) {
    static const char hex_chars[] = "0123456789ABCDEF";
    
    char buffer[12];
    int i = 0;
    
    /* Convert to hex string */
    do {
        buffer[i++] = hex_chars[value & 0xF];
        value >>= 4;
    } while (value > 0);
    
    /* Pad with zeros if needed */
    while (i < width && i < 8) {
        buffer[i++] = '0';
    }
    
    /* Print in reverse order */
    while (i > 0) {
        serial_putc(buffer[--i]);
    }
}

void serial_putdec(int32_t value) {
    if (value < 0) {
        serial_putc('-');
        value = -value;
    }
    
    char buffer[12];
    int i = 0;
    
    /* Convert to decimal string */
    do {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    } while (value > 0);
    
    /* Print in reverse order */
    while (i > 0) {
        serial_putc(buffer[--i]);
    }
}
