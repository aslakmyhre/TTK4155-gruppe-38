#include "uart.h"
#include <stdint.h>
#include <stdio.h>

#include <avr/io.h>
#include <util/delay.h>

//bit stream input
static int uart_putchar(char c, FILE *stream) {
    if (c == '\n') {
        uart_transmit('\r'); //for new-line support
    }

    uart_transmit(c);
    return 0;
}

//bit stream output
static int uart_getchar(FILE *stream) {
    return uart_recieve();
}

//initialize UART
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

//transmits
void uart_transmit(unsigned char data) {
    while (!(UCSR0A & (1 << UDRE0))) {}
    UDR0 = data; 
}

//recieves
unsigned char uart_receive(void) {
    while (!(UCSR0A & (1 << RXC0))) {}
    return UDR0; 
}