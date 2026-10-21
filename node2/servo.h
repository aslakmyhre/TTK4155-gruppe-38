#ifndef SERVO_H
#define SERVO_H

#include <stdint.h>

// Servo driver on top of the PWM driver (PB13, PWM channel 1).
// Position is given in percent, -100 (full one way) to 100 (full other way),
// the same scale node 1 uses for the joystick.

void servo_init(void);

// Out-of-range input is clamped to -100..100 here, and the PWM driver clamps
// the pulse to 0.9-2.1 ms again, so the servo can never get an invalid pulse.
void servo_set_position(int8_t percent);

#endif
