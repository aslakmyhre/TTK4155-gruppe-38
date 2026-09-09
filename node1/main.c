#define F_CPU 4915200UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "uart.h"


int main(void) {
    DDRA = 0xFF; //PA0-PA7 as outputs
    uart_init(UBRR_VALUE(9600));

    unsigned n = 0;

    while (1) {
        PORTA ^= 0xFF;

        _delay_ms(500); //blinking

        putchar(getchar()); //mirror input

        //printf("blink %u\n", n++); //counter
    }
}
