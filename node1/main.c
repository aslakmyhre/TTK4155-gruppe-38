#define F_CPU 4915200UL
#include <avr/io.h>
#include <stdio.h>
#include "uart.h"
#include "sram.h"


int main(void) {
    uart_init(UBRR_VALUE(9600));

    // PORTA/PORTC become the multiplexed address/data bus after this,
    // so no LED blinking on PA0 any more.
    sram_init();
    sram_test();

    while (1) {
        putchar(getchar()); //mirror input
    }
}
