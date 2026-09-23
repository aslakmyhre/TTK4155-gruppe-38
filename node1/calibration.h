#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <stdint.h>
#include "adc.h"

// ADC channel wiring
#define TOUCHPAD_X_CHANNEL 0
#define TOUCHPAD_Y_CHANNEL 1
#define JOYSTICK_X_CHANNEL 2
#define JOYSTICK_Y_CHANNEL 3

// digital values from 0..255, middle position at 128
#define CALIBRATION_ORIGO 128
#define CALIBRATION_MAX 255

// raw readings at extremes and middle position
// low:0, high:255
struct axis_calibration {
    uint8_t low;
    uint8_t center;
    uint8_t high;
};

void calibration_run(struct axis_calibration cal[ADC_NUM_CHANNELS]);

// scales input
uint8_t calibration_apply(const struct axis_calibration *cal, uint8_t raw);

#endif
