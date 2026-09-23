#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <stdint.h>

// Below this deflection on both axes the joystick counts as released
#define JOYSTICK_DEADZONE_PERCENT 20

// Percent deflection from rest, -100 (left/down) to 100 (right/up)
struct joystick_position {
    int8_t x;
    int8_t y;
};

enum joystick_direction {
    JOYSTICK_NEUTRAL,
    JOYSTICK_LEFT,
    JOYSTICK_RIGHT,
    JOYSTICK_UP,
    JOYSTICK_DOWN,
};

// Takes calibrated readings (0-255, rest at 128), see calibration_apply().
struct joystick_position joystick_position(uint8_t x, uint8_t y);

// Prints e.g. "X:  87%  Y: -21%" without a trailing newline.
void joystick_print_position(struct joystick_position pos);

// The axis with the larger deflection decides the direction.
enum joystick_direction joystick_direction(struct joystick_position pos);

// Returns a string in flash: print it with printf_P and %S.
const char *joystick_direction_name(enum joystick_direction direction);

#endif
