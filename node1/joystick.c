#include "joystick.h"
#include "calibration.h"

#include <stdio.h>
#include <stdlib.h>

#define PERCENT_MAX 100

// value to percent, accounting for uneven split (128 below origo, 127 above)
static int8_t to_percent(uint8_t value) {
    int16_t offset = (int16_t) value - CALIBRATION_ORIGO;
    int16_t span = offset < 0 ? CALIBRATION_ORIGO : CALIBRATION_MAX - CALIBRATION_ORIGO;
    return (int8_t) (offset * PERCENT_MAX / span);
}

struct joystick_position joystick_position(uint8_t x, uint8_t y) {
    struct joystick_position pos = { .x = to_percent(x), .y = to_percent(y) };
    return pos;
}

void joystick_print_position(struct joystick_position pos) {
    printf("X:%4d%%  Y:%4d%%", pos.x, pos.y);
}

enum joystick_direction joystick_direction(struct joystick_position pos) {
    int x_magnitude = abs(pos.x);
    int y_magnitude = abs(pos.y);

    if (x_magnitude < JOYSTICK_DEADZONE_PERCENT && y_magnitude < JOYSTICK_DEADZONE_PERCENT) {
        return JOYSTICK_NEUTRAL;
    }
    if (x_magnitude >= y_magnitude) {
        return pos.x < 0 ? JOYSTICK_LEFT : JOYSTICK_RIGHT;
    }
    return pos.y < 0 ? JOYSTICK_DOWN : JOYSTICK_UP;
}

const char *joystick_direction_name(enum joystick_direction direction) {
    switch (direction) {
        case JOYSTICK_NEUTRAL: return "NEUTRAL";
        case JOYSTICK_LEFT:    return "LEFT";
        case JOYSTICK_RIGHT:   return "RIGHT";
        case JOYSTICK_UP:      return "UP";
        case JOYSTICK_DOWN:    return "DOWN";
    }
    return "INVALID";
}
