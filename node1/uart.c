#include "uart.h"
#include <stdio.h>
#include <avr/io.h>

/*
* AVR's standard library expect this specific argument signature.
*/
static int uart_putchar(char c, FILE *stream) {
    /* since we always write to same UART we dont need to configure this
        to make sure the compiler doesnt complain */
    (void)stream;
    if (c == '\n') {
        uart_transmit('\r'); // terminals expect CRLF
    }

    uart_transmit(c);
    return 0;
}

/*
* Same as in uart_puchar, FDEV expect this signature
*/
static int uart_getchar(FILE *stream) {
    /* as noted earlier we always write to the same UART 
        so we just make compiler not complain */
    (void)stream;
    return uart_receive();
}


/*
* AVR's library macro for settitn up an in- and output stream.
* FDEV_SETUP_STREAM saves the function pointers needed for where to send/receive UART.
*
* Static instead of fdevopen(), which would link in malloc
*/
static FILE uart_stream = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);


/*
* Setup UART0. Takes ubrr (USART baud-rate register value) as an argument
* To calculate UBRR: f_osc/(16*BAUD)-1 
*/
void uart_init(uint16_t ubrr) {
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;

    // enable Rx and Tx on USART0
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);

    // 8 data bits, no parity, 1 stop bit. URSEL0 selects UCSR0C over UBRR0H.
    UCSR0C = (1 << URSEL0) | (1 << UCSZ01) | (1 << UCSZ00);

    // Route stdio through UART so printf/scanf work
    stdin = stdout = stderr = &uart_stream;
}

void uart_transmit(unsigned char data) {
    // check USART0 control and status register A's data-empty register
    while (!(UCSR0A & (1 << UDRE0))) {}
    UDR0 = data; // USART0 I/O data register
}

unsigned char uart_receive(void) {
    // check if USART0's I/O register has new data
    while (!(UCSR0A & (1 << RXC0))) {}
    return UDR0; // get Rx
}
