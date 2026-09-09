#define F_CPU 4915200UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdint.h>
#include "uart.h"

#define LED_ON_MS 300
#define EXT_BASE  0x1000 //above internal SRAM (ends 0x04FF), inside the ADC/SRAM window

int main(void) {
    MCUCR |= (1 << SRE); //external memory interface on: PA0-7 become AD0-7, PC pins become A8-15
    uart_init(UBRR_VALUE(9600));

    while (1) {
        for (uint8_t pin = 0; pin < 8; pin++) {
            uint16_t addr = EXT_BASE | (1 << pin);
            volatile uint8_t *ext = (volatile uint8_t *)addr;

            *ext = 0x00; //value irrelevant, the latch holds the low address byte A0-A7
            
            _delay_ms(LED_ON_MS);
        }
    }
}
