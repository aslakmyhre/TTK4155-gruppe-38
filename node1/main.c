#define F_CPU 4915200UL
#include <avr/io.h>
#include <stdio.h>
#include "uart.h"
#include "sram.h"


int main(void) {
    uart_init(UBRR_VALUE(9600));
    printf("UART up\n");

    // PORTA/PORTC become the multiplexed address/data bus after this,
    // so no LED blinking on PA0 any more.
    sram_init();
    printf("External memory interface enabled\n");

    // Run on demand so the test output is not missed when the terminal
    // connects after reset.
    while (1) {
        printf("Press any key to run the SRAM test\n");
        getchar();
        sram_test();
    }
}
