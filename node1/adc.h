#ifndef ADC_H
#define ADC_H

#include <stdint.h>

// MAX156 has 4 inputs: AIN0-AIN3
#define ADC_NUM_CHANNELS 4

// Starts a square wave on PD5 (OC1A) to drive the MAX156 CLK pin.
// Assumes MODE and VSS are tied to ground: single-ended, unipolar conversion.
void adc_init(void);

// Converts all channels and stores the results, index = channel number.
// Requires xmem_init() to have been called.
void adc_read(uint8_t values[ADC_NUM_CHANNELS]);

#endif
