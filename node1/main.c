#define F_CPU 4915200UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "uart.h"

#define LED_ON_MS 300

int main(void) {
    DDRA = 0xFF; //PA0-PA7 as outputs
    uart_init(UBRR_VALUE(9600));

    while (1) {
        for (uint8_t pin = 0; pin < 8; pin++) {
            PORTA = (1 << pin); //only this pin high
            printf("PA%u\n", pin);
            _delay_ms(LED_ON_MS);
        }
    }
}
