#ifndef UART_H
#define UART_H

#include <stdint.h>

void uart_init(uint16_t ubrr);
void uart_transmit(unsigned char data);
unsigned char uart_recieve(void);

// Baud rate
#define UBRR_VALUE(BAUD) ((F_CPU / (16UL * (BAUD))) - 1)


// From avrlib manual 
//static int uart_putchar(char c, FILE *stream); 
//static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE); 


#endif