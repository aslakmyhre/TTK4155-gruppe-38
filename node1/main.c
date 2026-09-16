#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include "adc.h"
#include "uart.h"
#include "sram.h"
#include "xmem.h"


int main(void) {
    uart_init(UBRR_VALUE(9600));

    // PORTA/PORTC become the multiplexed address/data bus after this,
    // so no LED blinking on PA0 any more.
    xmem_init();
    adc_init();
    sram_test();

    uint8_t adc_values[ADC_NUM_CHANNELS];
    while (1) {
        adc_read(adc_values);
        printf("ADC: %3u %3u %3u %3u\n",
               adc_values[0], adc_values[1], adc_values[2], adc_values[3]);
        _delay_ms(200);
    }
}
