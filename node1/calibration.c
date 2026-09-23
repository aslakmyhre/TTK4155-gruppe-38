#include "calibration.h"
#include "uart.h"

#include <stdbool.h>
#include <stdio.h>


static void read_on_enter(const char *prompt, uint8_t values[ADC_NUM_CHANNELS]) {
    printf("%s, then press Enter\n", prompt);
    uart_receive(); //any key works
    adc_read(values);
}

static uint8_t read_channel_on_enter(const char *prompt, uint8_t channel) {
    //read values when pressed
    uint8_t values[ADC_NUM_CHANNELS];
    read_on_enter(prompt, values);
    printf("  ch%u = %u\n", channel, values[channel]);
    return values[channel];
}

// calibration_apply() divides by the distance to center on each side, and
// assumes low and high lie on opposite sides of it.
static bool center_between_extremes(const struct axis_calibration *cal) {
    return (cal->low < cal->center && cal->center < cal->high) || (cal->low > cal->center && cal->center > cal->high);
}

// The touchpad has no rest position, so its center is derived from the extremes.
static void calibrate_axis(struct axis_calibration *cal, uint8_t channel, const char *low_prompt, const char *high_prompt, bool center_from_extremes) {
    while (1) {
        cal->low = read_channel_on_enter(low_prompt, channel);
        cal->high = read_channel_on_enter(high_prompt, channel);
        if (center_from_extremes) {
            cal->center = ((uint16_t) cal->low + cal->high) / 2;
        }
        if (center_between_extremes(cal)) {
            return;
        }
        printf("ERROR: ch%u low=%u center=%u high=%u, center must lie "
               "between the extremes. Try again.\n",
               channel, cal->low, cal->center, cal->high);
    }
}

static void calibrate_joystick(struct axis_calibration cal[ADC_NUM_CHANNELS]) {
    uint8_t rest[ADC_NUM_CHANNELS];
    read_on_enter("Release the joystick", rest);
    cal[JOYSTICK_X_CHANNEL].center = rest[JOYSTICK_X_CHANNEL];
    cal[JOYSTICK_Y_CHANNEL].center = rest[JOYSTICK_Y_CHANNEL];
    printf("  ch%u = %u, ch%u = %u\n",
           JOYSTICK_X_CHANNEL, rest[JOYSTICK_X_CHANNEL],
           JOYSTICK_Y_CHANNEL, rest[JOYSTICK_Y_CHANNEL]);

    calibrate_axis(&cal[JOYSTICK_Y_CHANNEL], JOYSTICK_Y_CHANNEL,
                   "Pull the joystick DOWN", "Push the joystick UP", false);
    calibrate_axis(&cal[JOYSTICK_X_CHANNEL], JOYSTICK_X_CHANNEL,
                   "Push the joystick LEFT", "Push the joystick RIGHT", false);
}

static void calibrate_touchpad(struct axis_calibration cal[ADC_NUM_CHANNELS]) {
    calibrate_axis(&cal[TOUCHPAD_Y_CHANNEL], TOUCHPAD_Y_CHANNEL,
                   "Hold the touchpad at the BOTTOM", "Hold the touchpad at the TOP",
                   true);
    calibrate_axis(&cal[TOUCHPAD_X_CHANNEL], TOUCHPAD_X_CHANNEL,
                   "Hold the touchpad at the LEFT", "Hold the touchpad at the RIGHT",
                   true);
}

void calibration_run(struct axis_calibration cal[ADC_NUM_CHANNELS]) {
    printf("\n--- Joystick calibration ---\n");
    calibrate_joystick(cal);
    printf("\n--- Touchpad calibration ---\n");
    calibrate_touchpad(cal);
    printf("\nCalibration done\n");
    for (uint8_t channel = 0; channel < ADC_NUM_CHANNELS; channel++) {
        printf("  ch%u: low=%3u center=%3u high=%3u\n",
               channel, cal[channel].low, cal[channel].center, cal[channel].high);
    }
}

uint8_t calibration_apply(const struct axis_calibration *cal, uint8_t raw) {
    int16_t offset = (int16_t) raw - cal->center;
    int16_t low_span = (int16_t) cal->low - cal->center;
    int16_t high_span = (int16_t) cal->high - cal->center;

    // Comparing signs instead of magnitudes keeps inverted wiring working
    int32_t scaled;
    if (offset == 0) {
        scaled = CALIBRATION_ORIGO;
    } else if ((offset < 0) == (low_span < 0)) {
        scaled = CALIBRATION_ORIGO - (int32_t) CALIBRATION_ORIGO * offset / low_span;
    } else {
        scaled = CALIBRATION_ORIGO
               + (int32_t) (CALIBRATION_MAX - CALIBRATION_ORIGO) * offset / high_span;
    }

    if (scaled < 0) {
        return 0;
    }
    if (scaled > CALIBRATION_MAX) {
        return CALIBRATION_MAX;
    }
    return (uint8_t) scaled;
}
