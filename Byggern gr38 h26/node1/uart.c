#include "uart.h"
#include <stdint.h>
#include <stdio.h>

#include <avr/io.h>
#include <util/delay.h>



#define USART3_BAUD_RATE(BAUD_RATE) ((float)(F_CPU * 64 / (16 *(float)BAUD_RATE)) + 0.5)

// From avrlib manual 
static int uart_putchar(char c, FILE *stream); 
static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL); 


void uart_init(uint16_t ubrr) {
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    UCSR0C = (1 << URSEL0) | (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_transmit(unsigned char data) {

}

unsigned char uart_recieve(void) {

}