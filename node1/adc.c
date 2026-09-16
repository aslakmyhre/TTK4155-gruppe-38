#include "adc.h"

#include <avr/io.h>

// f_clk = F_CPU / (2 * prescaler * (1 + OCR1A)) = 2.4576 MHz, the fastest
// Timer1 can toggle. MAX156 accepts 0.5-5.0 MHz on CLK.
#define ADC_CLOCK_OCR 0


void adc_clock_init(void) {
    DDRD |= (1 << PD5);

    // Toggling OC1A in hardware on every compare match gives a 50% duty
    // cycle clock that keeps running without any CPU involvement.
    TCCR1A |= (1 << COM1A0);
    OCR1A = ADC_CLOCK_OCR;
    // CTC mode 4 (TOP = OCR1A), no prescaler. Setting CS10 starts the timer.
    TCCR1B |= (1 << WGM12) | (1 << CS10);
}
