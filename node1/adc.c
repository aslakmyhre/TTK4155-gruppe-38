#include "adc.h"
#include "xmem.h"

#include <avr/io.h>
#include <util/delay.h>

// f_clk = F_CPU / (2 * prescaler * (1 + OCR1A)) = 2.4576 MHz
#define ADC_CLOCK_OCR 0
#define ADC_CLOCK_HZ (F_CPU / (2UL * (1 + ADC_CLOCK_OCR)))

// wait for 8 channels converted  TODO: test 4
#define ADC_CONVERTED_CHANNELS 8

// tCONV = (9 * N * 2) / f_clk
#define ADC_CONVERSION_US \
    ((9UL * ADC_CONVERTED_CHANNELS * 2 * 1000000UL) / ADC_CLOCK_HZ + 1)

#define ADC_START_CONVERSION 0

static volatile uint8_t *const adc = (uint8_t *) ADC_BASE;

void adc_init(void) {
    // ADC clock
    DDRD |= (1 << PD5);
    TCCR1A |= (1 << COM1A0);
    OCR1A = ADC_CLOCK_OCR;
    TCCR1B |= (1 << WGM12) | (1 << CS10);
}

void adc_read(uint8_t values[ADC_NUM_CHANNELS]) {
    // RAM read pointer resets to 0
    adc[0] = ADC_START_CONVERSION;

    // BUSY not wired, wait
    _delay_us(ADC_CONVERSION_US);

    // RD returns the next channel, starting from channel 0
    for (uint8_t channel = 0; channel < ADC_NUM_CHANNELS; channel++) {
        values[channel] = adc[0];
    }
}
