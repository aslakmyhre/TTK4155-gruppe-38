#ifndef ADC_H
#define ADC_H

#include <stdint.h>

// MAX156 has 4 inputs: AIN0-AIN3
#define ADC_NUM_CHANNELS 4

// Set clock pin
void adc_init(void);

void adc_read(uint8_t values[ADC_NUM_CHANNELS]);

#endif
