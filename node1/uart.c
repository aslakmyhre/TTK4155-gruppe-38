#include "uart.h"
#include <stdio.h>

#include <avr/io.h>

static int uart_putchar(char c, FILE *stream) {
    (void)stream;
    if (c == '\n') {
        uart_transmit('\r'); // terminals expect CRLF
    }

    uart_transmit(c);
    return 0;
}

static int uart_getchar(FILE *stream) {
    (void)stream;
    return uart_receive();
}

// Static instead of fdevopen(), which would link in malloc
static FILE uart_stream = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

void uart_init(uint16_t ubrr) {
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;

    UCSR0B = (1 << RXEN0) | (1 << TXEN0);

    // 8 data bits, no parity, 1 stop bit. URSEL0 selects UCSR0C over UBRR0H.
    UCSR0C = (1 << URSEL0) | (1 << UCSZ01) | (1 << UCSZ00);

    // Route stdio through UART so printf/scanf work
    stdin = stdout = stderr = &uart_stream;
}

void uart_transmit(unsigned char data) {
    while (!(UCSR0A & (1 << UDRE0))) {}
    UDR0 = data;
}

unsigned char uart_receive(void) {
    while (!(UCSR0A & (1 << RXC0))) {}
    return UDR0;
}
