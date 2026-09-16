#include "adc.h"
#include "xmem.h"

#include <avr/io.h>
#include <util/delay.h>

// f_clk = F_CPU / (2 * prescaler * (1 + OCR1A)) = 2.4576 MHz, the fastest
// Timer1 can toggle. MAX156 accepts 0.5-5.0 MHz on CLK.
#define ADC_CLOCK_OCR 0
#define ADC_CLOCK_HZ (F_CPU / (2UL * (1 + ADC_CLOCK_OCR)))

// With MODE tied low, datasheet Table 4 only describes the MAX155's
// 8-channel conversion, so wait as if all 8 are converted.
#define ADC_CONVERTED_CHANNELS 8

// Datasheet: tCONV = (9 * N * 2) / f_clk, including one clock of
// uncertainty. The extra microsecond rounds the integer division up.
#define ADC_CONVERSION_US \
    ((9UL * ADC_CONVERTED_CHANNELS * 2 * 1000000UL) / ADC_CLOCK_HZ + 1)

// Hard-wired mode ignores the data bus on WR, so any value starts a conversion.
#define ADC_START_CONVERSION 0


static volatile uint8_t *const adc = (uint8_t *) ADC_BASE;


void adc_init(void) {
    DDRD |= (1 << PD5);

    // Toggling OC1A in hardware on every compare match gives a 50% duty
    // cycle clock that keeps running without any CPU involvement.
    TCCR1A |= (1 << COM1A0);
    OCR1A = ADC_CLOCK_OCR;
    // CTC mode 4 (TOP = OCR1A), no prescaler. Setting CS10 starts the timer.
    TCCR1B |= (1 << WGM12) | (1 << CS10);
}

void adc_read(uint8_t values[ADC_NUM_CHANNELS]) {
    // All channels are sampled on WR's falling edge, which also resets the
    // RAM read pointer to channel 0.
    adc[0] = ADC_START_CONVERSION;

    // BUSY is not wired, so wait out the worst-case conversion time instead.
    _delay_us(ADC_CONVERSION_US);

    // Each RD returns the next channel, starting from channel 0.
    for (uint8_t channel = 0; channel < ADC_NUM_CHANNELS; channel++) {
        values[channel] = adc[0];
    }
}
