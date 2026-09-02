#include "uart.h"
#include <stdint.h>
#include <stdio.h>

#include <avr/io.h>
#include <util/delay.h>




static int uart_putchar(char c, FILE *stream) {
    // Terminals expect CRLF, C only emits LF
    if (c == '\n') {
        uart_transmit('\r');
    }
    uart_transmit(c);
    return 0;
}

static int uart_getchar(FILE *stream) {
    return uart_recieve();
}

void uart_init(uint16_t ubrr) {
    //split UBRRHI register
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;

    // Init for TX & RX 
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);

    // Init for timer count
    UCSR0C = (1 << URSEL0) | (1 << UCSZ01) | (1 << UCSZ00);

    // Route stdin/stdout through UART so printf/scanf work
    fdevopen(uart_putchar, uart_getchar);
}

void uart_transmit(unsigned char data) {
    while (!(UCSR0A & (1 << UDRE0))) {}
    UDR0 = data; 
}

unsigned char uart_recieve(void) {
    while (!(UCSR0A & (1 << RXC0))) {}
    return UDR0; 
}