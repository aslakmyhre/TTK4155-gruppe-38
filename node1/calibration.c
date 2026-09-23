#include "calibration.h"
#include "io_board.h"
#include "oled.h"

#include <avr/pgmspace.h>
#include <stdbool.h>
#include <stdio.h>
#include <util/delay.h>

#define BUTTON_POLL_MS 20


// Raw masks: any bit counts, since the bit-to-button mapping is unverified
static bool button_down(void) {
    struct io_buttons buttons = io_board_buttons();
    return (buttons.right | buttons.left) != 0;
}

// Waits for a release first, so one long press can't confirm several steps
static void wait_for_button(void) {
    while (button_down()) {
        _delay_ms(BUTTON_POLL_MS);
    }
    while (!button_down()) {
        _delay_ms(BUTTON_POLL_MS);
    }
}

// Prompts are flash strings (PSTR) to keep them out of the 1 KiB internal RAM.
// They must fit one 21-character OLED line.
static void read_on_press(const char *prompt, uint8_t values[ADC_NUM_CHANNELS]) {
    oled_clear();
    fprintf_P(oled_output(), PSTR("CALIBRATION\n\n%S\n\nTHEN PRESS A BUTTON"), prompt);
    wait_for_button();
    adc_read(values);
}

static uint8_t read_channel_on_press(const char *prompt, uint8_t channel) {
    uint8_t values[ADC_NUM_CHANNELS];
    read_on_press(prompt, values);
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
        cal->low = read_channel_on_press(low_prompt, channel);
        cal->high = read_channel_on_press(high_prompt, channel);
        if (center_from_extremes) {
            cal->center = ((uint16_t) cal->low + cal->high) / 2;
        }
        if (center_between_extremes(cal)) {
            return;
        }
        oled_clear();
        fprintf_P(oled_output(),
                  PSTR("CALIBRATION ERROR\n\nCH%u L%u C%u H%u\nCENTER MUST LIE\n"
                       "BETWEEN EXTREMES\n\nPRESS TO RETRY"),
                  channel, cal->low, cal->center, cal->high);
        wait_for_button();
    }
}

static void calibrate_joystick(struct axis_calibration cal[ADC_NUM_CHANNELS]) {
    uint8_t rest[ADC_NUM_CHANNELS];
    read_on_press(PSTR("RELEASE THE JOYSTICK"), rest);
    cal[JOYSTICK_X_CHANNEL].center = rest[JOYSTICK_X_CHANNEL];
    cal[JOYSTICK_Y_CHANNEL].center = rest[JOYSTICK_Y_CHANNEL];

    calibrate_axis(&cal[JOYSTICK_Y_CHANNEL], JOYSTICK_Y_CHANNEL,
                   PSTR("PULL JOYSTICK DOWN"), PSTR("PUSH JOYSTICK UP"), false);
    calibrate_axis(&cal[JOYSTICK_X_CHANNEL], JOYSTICK_X_CHANNEL,
                   PSTR("PUSH JOYSTICK LEFT"), PSTR("PUSH JOYSTICK RIGHT"), false);
}

static void calibrate_touchpad(struct axis_calibration cal[ADC_NUM_CHANNELS]) {
    calibrate_axis(&cal[TOUCHPAD_Y_CHANNEL], TOUCHPAD_Y_CHANNEL,
                   PSTR("HOLD TOUCHPAD BOTTOM"), PSTR("HOLD TOUCHPAD TOP"),
                   true);
    calibrate_axis(&cal[TOUCHPAD_X_CHANNEL], TOUCHPAD_X_CHANNEL,
                   PSTR("HOLD TOUCHPAD LEFT"), PSTR("HOLD TOUCHPAD RIGHT"),
                   true);
}

void calibration_run(struct axis_calibration cal[ADC_NUM_CHANNELS]) {
    calibrate_joystick(cal);
    calibrate_touchpad(cal);

    FILE *out = oled_output();
    oled_clear();
    fprintf_P(out, PSTR("CALIBRATION DONE\n\n"));
    for (uint8_t channel = 0; channel < ADC_NUM_CHANNELS; channel++) {
        fprintf_P(out, PSTR("CH%u L%3u C%3u H%3u\n"),
                  channel, cal[channel].low, cal[channel].center, cal[channel].high);
    }
    fprintf_P(out, PSTR("\nPRESS TO CONTINUE"));
    wait_for_button();
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
