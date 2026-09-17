#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <stdint.h>
#include "adc.h"

// ADC channel wiring, see tools/README.md
#define TOUCHPAD_X_CHANNEL 0
#define TOUCHPAD_Y_CHANNEL 1
#define JOYSTICK_X_CHANNEL 2
#define JOYSTICK_Y_CHANNEL 3

// Calibrated values span 0-255 with the rest position at 128
#define CALIBRATION_ORIGO 128
#define CALIBRATION_MAX 255

// Raw readings at the axis extremes and at its rest position.
// low maps to 0 and high to 255, whichever raw value is larger.
struct axis_calibration {
    uint8_t low;
    uint8_t center;
    uint8_t high;
};

// Walks the user through the joystick and touchpad extremes over UART.
// Blocks until every step is confirmed with Enter.
// Index = ADC channel number.
void calibration_run(struct axis_calibration cal[ADC_NUM_CHANNELS]);

// Scales a raw reading to 0-255, clamping values beyond the calibrated extremes.
uint8_t calibration_apply(const struct axis_calibration *cal, uint8_t raw);

#endif
