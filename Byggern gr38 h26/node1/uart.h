#ifndef UART_H
#define UART_H

#include <stdint.h>

void uart_init(uint16_t ubrr);
void uart_transmit(unsigned char data);
unsigned char uart_recieve(void);

#endif