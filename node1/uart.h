#ifndef UART_H
#define UART_H

#include <stdbool.h>
#include <stdint.h>

void uart_init(uint16_t ubrr);
void uart_transmit(unsigned char data);
unsigned char uart_receive(void);
// True if a received byte is waiting, without blocking
bool uart_has_data(void);

// Baud rate
#define UBRR_VALUE(BAUD) ((F_CPU / (16UL * (BAUD))) - 1)

#endif
