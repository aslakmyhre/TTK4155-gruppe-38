#define F_CPU 4915200UL
#include <avr/io.h>
#include <util/delay.h>
#include "uart.h"


int main(void) {
    DDRA |= (1<<PA0); // PA0
    uart_init(UBRR_VALUE(9600));

    while (1) {
        PORTA ^= (1<<PA0);
        
        _delay_ms(500);
        uart_transmit('c');
    }
}
