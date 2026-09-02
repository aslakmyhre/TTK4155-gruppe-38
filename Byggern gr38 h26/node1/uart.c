#include "uart.h"
#include <stdint.h>
#include <stdio.h>

#include <avr/io.h>
#include <util/delay.h>




void uart_init(uint16_t ubrr) {
    //split UBRRHI register
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;

    // Init for TX & RX 
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);

    // Init for timer count
    UCSR0C = (1 << URSEL0) | (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_transmit(unsigned char data) {
    while (!(UCSR0A & (1 << UDRE0))) {}
    UDR0 = data; 
}

unsigned char uart_recieve(void) {
    while (!(UCSR0A & (1 << RXC0))) {}
    return UDR0; 
}