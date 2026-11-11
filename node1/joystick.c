#include "joystick.h"
#include "calibration.h"

#include <avr/pgmspace.h>
#include <stdio.h>
#include <stdlib.h>

#define PERCENT_MAX 100


// The origo splits 0-255 unevenly (128 below, 127 above), so each side is
// scaled on its own to reach exactly -100 and 100.
static int8_t to_percent(uint8_t value) {
    int16_t offset = (int16_t) value - CALIBRATION_ORIGO;
    int16_t span = offset < 0 ? CALIBRATION_ORIGO : CALIBRATION_MAX - CALIBRATION_ORIGO;
    return (int8_t) (offset * PERCENT_MAX / span);
}

//convert calibrated x and y (0-255) to percent (-100 to 100)
struct joystick_position joystick_position(uint8_t x, uint8_t y) {
    struct joystick_position pos = { .x = to_percent(x), .y = to_percent(y) };
    return pos;
}

//print position over UART
void joystick_print_position(struct joystick_position pos) {
    printf_P(PSTR("X:%4d%%  Y:%4d%%"), pos.x, pos.y);
}

//find direction from position, neutral inside deadzone
enum joystick_direction joystick_direction(struct joystick_position pos) {
    int x_magnitude = abs(pos.x);
    int y_magnitude = abs(pos.y);

    //both axes inside deadzone
    if (x_magnitude < JOYSTICK_DEADZONE_PERCENT && y_magnitude < JOYSTICK_DEADZONE_PERCENT) {
        return JOYSTICK_NEUTRAL;
    }
    //axis with largest deflection decides direction
    if (x_magnitude >= y_magnitude) {
        return pos.x < 0 ? JOYSTICK_LEFT : JOYSTICK_RIGHT;
    }
    return pos.y < 0 ? JOYSTICK_DOWN : JOYSTICK_UP;
}

//direction as text, stored in flash
const char *joystick_direction_name(enum joystick_direction direction) {
    switch (direction) {
        case JOYSTICK_NEUTRAL: return PSTR("NEUTRAL");
        case JOYSTICK_LEFT:    return PSTR("LEFT");
        case JOYSTICK_RIGHT:   return PSTR("RIGHT");
        case JOYSTICK_UP:      return PSTR("UP");
        case JOYSTICK_DOWN:    return PSTR("DOWN");
    }
    return PSTR("INVALID");
}
