#ifndef INPUT_DISPLAY_H
#define INPUT_DISPLAY_H

#include <stdint.h>
#include "adc.h"

// Draws the joystick (left half) and touchpad (right half) as a dot inside a
// frame, with their values underneath, and sends the frame to the display.
// Takes calibrated readings (0-255), index = ADC channel number.
void input_display_show(const uint8_t values[ADC_NUM_CHANNELS]);

#endif