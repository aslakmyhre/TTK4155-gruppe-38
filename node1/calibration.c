#include "calibration.h"
#include "io_board.h"
#include "oled.h"

#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <stdbool.h>
#include <stdio.h>
#include <util/delay.h>

#define BUTTON_POLL_MS 20
#define DEFAULT_LOW     0
#define DEFAULT_HIGH    CALIBRATION_MAX
#define EEPROM_CALIBRATION_MARKER 0xA5

static struct {
    uint8_t marker;
    struct axis_calibration axes[ADC_NUM_CHANNELS];
} eeprom_calibration EEMEM;


static bool load_calibration(struct axis_calibration cal[ADC_NUM_CHANNELS]) {
    if (eeprom_read_byte(&eeprom_calibration.marker) != EEPROM_CALIBRATION_MARKER) {
        return false;
    }
    eeprom_read_block(cal, eeprom_calibration.axes, sizeof eeprom_calibration.axes);
    return true;
}

static void save_calibration(const struct axis_calibration cal[ADC_NUM_CHANNELS]) {
    // Mark the record invalid until all values have been written.
    eeprom_update_byte(&eeprom_calibration.marker, 0);
    eeprom_update_block(cal, eeprom_calibration.axes, sizeof eeprom_calibration.axes);
    eeprom_update_byte(&eeprom_calibration.marker, EEPROM_CALIBRATION_MARKER);
    eeprom_busy_wait();
}


// Raw masks: any bit counts, since the bit-to-button mapping is unverified
static bool button_down(void) {
    struct io_buttons buttons = io_board_buttons();
    return (buttons.right | buttons.left) != 0;
}

static void wait_for_release(void) {
    while (button_down()) {
        _delay_ms(BUTTON_POLL_MS);
    }
}

// Waits for a release first, so one long press can't confirm several steps
static void wait_for_button(void) {
    wait_for_release();
    while (!button_down()) {
        _delay_ms(BUTTON_POLL_MS);
    }
}

// Returns true for a left-hand button, false for a right-hand one
static bool wait_for_left_or_right(void) {
    wait_for_release();
    while (1) {
        struct io_buttons buttons = io_board_buttons();
        if (buttons.left) {
            return true;
        }
        if (buttons.right) {
            return false;
        }
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
    save_calibration(cal);
    printf_P(PSTR("calibration: saved to EEPROM\n"));

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

// Full ADC range with the rest position assumed at the midpoint. Wrong if an
// axis is wired inverted or its rest position is far off 128.
static void apply_defaults(struct axis_calibration cal[ADC_NUM_CHANNELS]) {
    for (uint8_t channel = 0; channel < ADC_NUM_CHANNELS; channel++) {
        cal[channel].low = DEFAULT_LOW;
        cal[channel].center = CALIBRATION_ORIGO;
        cal[channel].high = DEFAULT_HIGH;
    }
}

void calibration_ask(struct axis_calibration cal[ADC_NUM_CHANNELS]) {
    if (load_calibration(cal)) {
        printf_P(PSTR("calibration: loaded from EEPROM\n"));
    } else {
        apply_defaults(cal);
        printf_P(PSTR("calibration: no saved values, using defaults L%u C%u H%u\n"),
                 DEFAULT_LOW, CALIBRATION_ORIGO, DEFAULT_HIGH);
    }

    oled_clear();
    fprintf_P(oled_output(), PSTR("CALIBRATE?\n\nLEFT BUTTON:  YES\nRIGHT BUTTON: SKIP"));
    if (wait_for_left_or_right()) {
        calibration_run(cal);
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
