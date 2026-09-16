#include <avr/io.h>
#include <stdio.h>
#include "adc.h"
#include "uart.h"
#include "sram.h"
#include "xmem.h"


int main(void) {
    uart_init(UBRR_VALUE(9600));

    // PORTA/PORTC become the multiplexed address/data bus after this,
    // so no LED blinking on PA0 any more.
    xmem_init();
#ifdef ADC_CLOCK_DISABLED
    printf("ADC clock disabled\n");
#else
    adc_clock_init();
    printf("ADC clock on, OCR1A=%d\n", ADC_CLOCK_OCR);
#endif
    xmem_decode_test();
    sram_test();

    while (1) {
        putchar(getchar()); //mirror input
    }
}
