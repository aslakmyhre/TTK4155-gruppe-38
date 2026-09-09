#define F_CPU 4915200UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "uart.h"
#include "sram.h"


int main(void) {
    uart_init(UBRR_VALUE(9600));

    // Blink PA0 before the bus takes the port over: proof the MCU runs even
    // if the serial link is dead. One second per half period, so a clock that
    // is not the 4.9152 MHz crystal is visible by timing the blink.
    DDRA |= (1 << PA0);
    for (uint8_t i = 0; i < 20; i++) {
        PORTA ^= (1 << PA0);
        printf("UART up\n");
        _delay_ms(1000);
    }

    // PORTA/PORTC become the multiplexed address/data bus after this,
    // so no LED blinking on PA0 any more.
    sram_init();
    printf("External memory interface enabled\n");

    // Run on demand so the test output is not missed when the terminal
    // connects after reset.
    while (1) {
        printf("Press any key to run the SRAM test\n");
        getchar();
        sram_address_test();
        sram_test();
    }
}
