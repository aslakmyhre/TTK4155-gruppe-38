#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <stdint.h>

#define JOYSTICK_DEADZONE_PERCENT 20

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

struct joystick_position joystick_position(uint8_t x, uint8_t y);

void joystick_print_position(struct joystick_position pos);

enum joystick_direction joystick_direction(struct joystick_position pos);

const char *joystick_direction_name(enum joystick_direction direction);

#endif
