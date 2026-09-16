#include "adc.h"

#include <avr/io.h>

// f_clk = F_CPU / (2 * prescaler * (1 + OCR1A)). ADC_CLOCK_OCR comes from
// the Makefile so the frequency can be changed without editing code.
#ifndef ADC_CLOCK_OCR
#error "ADC_CLOCK_OCR must be defined (see Makefile)"
#endif


void adc_clock_init(void) {
    DDRD |= (1 << PD5);

    // Toggling OC1A in hardware on every compare match gives a 50% duty
    // cycle clock that keeps running without any CPU involvement.
    TCCR1A |= (1 << COM1A0);
    OCR1A = ADC_CLOCK_OCR;
    // CTC mode 4 (TOP = OCR1A), no prescaler. Setting CS10 starts the timer.
    TCCR1B |= (1 << WGM12) | (1 << CS10);
}
