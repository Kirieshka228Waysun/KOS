/* ============================================================================
 * KOS Bootloader - Serial Driver (COM1)
 * ============================================================================
 * UART 16550A serial driver for COM1
 * 9600 baud, 8N1, polling mode
 * ============================================================================
 */

#ifndef KOS_SERIAL_H
#define KOS_SERIAL_H

#include "types.h"

/* ============================================================================
 * Serial Port Configuration
 * ============================================================================
 */

/* COM1 base port address */
#define SERIAL_COM1_BASE    0x3F8

/* Default configuration */
#define SERIAL_BAUD_RATE    9600
#define SERIAL_DATA_BITS    8
#define SERIAL_PARITY       'N'   /* None */
#define SERIAL_STOP_BITS    1

/* ============================================================================
 * UART Register Offsets (from base port)
 * ============================================================================
 */

#define UART_RBR    0   /* Receiver Buffer Register (read) */
#define UART_THR    0   /* Transmitter Holding Register (write) */
#define UART_IER    1   /* Interrupt Enable Register */
#define UART_IIR    2   /* Interrupt Identification Register (read) */
#define UART_FCR    2   /* FIFO Control Register (write) */
#define UART_LCR    3   /* Line Control Register */
#define UART_MCR    4   /* Modem Control Register */
#define UART_LSR    5   /* Line Status Register */
#define UART_MSR    6   /* Modem Status Register */
#define UART_SCR    7   /* Scratch Register */
#define UART_DLL    0   /* Divisor Latch Low (when DLAB=1) */
#define UART_DLM    1   /* Divisor Latch High (when DLAB=1) */

/* ============================================================================
 * Register Bit Definitions
 * ============================================================================
 */

/* Line Control Register (LCR) bits */
#define UART_LCR_DLAB     0x80  /* Divisor Latch Access Bit */
#define UART_LCR_BREAK    0x40  /* Set Break */
#define UART_LCR_STICKY   0x20  /* Stick Parity */
#define UART_LCR_EVEN     0x10  /* Even Parity */
#define UART_LCR_PARITY   0x08  /* Parity Enable */
#define UART_LCR_STOP     0x04  /* Stop Bits (1=2 stop, 0=1 stop) */
#define UART_LCR_WLEN_8   0x03  /* Word Length: 8 bits */
#define UART_LCR_WLEN_7   0x02  /* Word Length: 7 bits */
#define UART_LCR_WLEN_6   0x01  /* Word Length: 6 bits */
#define UART_LCR_WLEN_5   0x00  /* Word Length: 5 bits */

/* Line Status Register (LSR) bits */
#define UART_LSR_ERROR    0x80  /* Error in receiver */
#define UART_LSR_TEMT     0x40  /* Transmitter Empty */
#define UART_LSR_THRE     0x20  /* Transmitter Hold Register Empty */
#define UART_LSR_BI       0x10  /* Break Interrupt */
#define UART_LSR_FE       0x08  /* Framing Error */
#define UART_LSR_PE       0x04  /* Parity Error */
#define UART_LSR_OE       0x02  /* Overrun Error */
#define UART_LSR_DR       0x01  /* Data Ready */

/* Modem Control Register (MCR) bits */
#define UART_MCR_LOOP     0x10  /* Loopback Mode */
#define UART_MCR_AUX2     0x08  /* Auxiliary Output 2 */
#define UART_MCR_AUX1     0x04  /* Auxiliary Output 1 */
#define UART_MCR_RTS      0x02  /* Request To Send */
#define UART_MCR_DTR      0x01  /* Data Terminal Ready */

/* ============================================================================
 * Function Prototypes
 * ============================================================================
 */

/**
 * @brief Initialize serial port COM1
 * Configures UART for 9600 baud, 8N1
 */
void serial_init(void);

/**
 * @brief Check if serial port is initialized
 * @return true if initialized, false otherwise
 */
bool serial_is_initialized(void);

/**
 * @brief Send a byte over serial (blocking)
 * @param c Byte to send
 */
void serial_putc(char c);

/**
 * @brief Send a string over serial
 * @param str Null-terminated string to send
 */
void serial_puts(const char* str);

/**
 * @brief Receive a byte from serial (blocking)
 * @return Received byte, or -1 if error
 */
int serial_getc(void);

/**
 * @brief Check if a byte is available to read (non-blocking)
 * @return true if data available, false otherwise
 */
bool serial_data_available(void);

/**
 * @brief Try to receive a byte (non-blocking)
 * @param c Pointer to store received byte
 * @return true if byte was read, false if no data
 */
bool serial_try_getc(char* c);

/**
 * @brief Flush transmit buffer
 * Waits until all bytes are transmitted
 */
void serial_flush(void);

/**
 * @brief Clear receive buffer
 * Discards any pending input
 */
void serial_clear_rx_buffer(void);

/**
 * @brief Write hex value to serial
 * @param value Value to print
 * @param width Number of hex digits (1-8)
 */
void serial_puthex(uint32_t value, int width);

/**
 * @brief Write decimal value to serial
 * @param value Value to print
 */
void serial_putdec(int32_t value);

#endif /* KOS_SERIAL_H */
