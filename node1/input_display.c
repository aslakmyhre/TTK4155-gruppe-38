#include "input_display.h"
#include "calibration.h"
#include "joystick.h"
#include "oled.h"

#include <stdio.h>

#define PANEL_SIZE 50
#define DOT_SIZE   3
#define TEXT_PAGE  7    // bottom row, below the panels

#define JOYSTICK_PANEL_X 7     // centered in the left half
#define TOUCHPAD_PANEL_X 71    // centered in the right half
#define JOYSTICK_TEXT_X  0
#define TOUCHPAD_TEXT_X  64


// Draws a frame with a dot at the calibrated (0-255) position.
static void draw_panel(uint8_t x0, uint8_t x_value, uint8_t y_value) {
    oled_rect(x0, 0, PANEL_SIZE, PANEL_SIZE);

    // Keeps the dot inside the frame border
    uint8_t travel = PANEL_SIZE - 2 - DOT_SIZE;
    uint8_t dot_x = x0 + 1 + (uint16_t) x_value * travel / CALIBRATION_MAX;
    // Screen y grows downward, so up on the stick must map to a smaller y
    uint8_t dot_y = 1 + (uint16_t) (CALIBRATION_MAX - y_value) * travel / CALIBRATION_MAX;
    oled_fill_rect(dot_x, dot_y, DOT_SIZE, DOT_SIZE);
}

// Prints "X<x> Y<y>" in a 64 pixel wide half of the screen.
static void draw_values(uint8_t x0, int16_t x, int16_t y) {
    char text[8];
    snprintf(text, sizeof text, "X%4d", x);
    oled_text(TEXT_PAGE, x0, text);
    snprintf(text, sizeof text, "Y%4d", y);
    oled_text(TEXT_PAGE, x0 + 32, text);
}

void input_display_show(const uint8_t values[ADC_NUM_CHANNELS]) {
    struct joystick_position pos = joystick_position(
        values[JOYSTICK_X_CHANNEL], values[JOYSTICK_Y_CHANNEL]);

    oled_fb_clear();

    // Joystick numbers in percent (-100 to 100)
    draw_panel(JOYSTICK_PANEL_X,
               values[JOYSTICK_X_CHANNEL], values[JOYSTICK_Y_CHANNEL]);
    draw_values(JOYSTICK_TEXT_X, pos.x, pos.y);

    // Touchpad numbers as calibrated 0-255
    draw_panel(TOUCHPAD_PANEL_X,
               values[TOUCHPAD_X_CHANNEL], values[TOUCHPAD_Y_CHANNEL]);
    draw_values(TOUCHPAD_TEXT_X,
                values[TOUCHPAD_X_CHANNEL], values[TOUCHPAD_Y_CHANNEL]);

    oled_flush();
}