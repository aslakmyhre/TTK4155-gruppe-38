#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

#define MOTOR_MAX_PCT   50   // speed limit, input above this is clamped

// Motor driver (phase/enable) on the motor shield.
//   ENABLE/PWM: PB12 (D20), PWM channel 0 (PWMH0), peripheral B
//   PHASE/DIR:  PC23 (D7)
//
// After reset every SAM3X pin is an input with its pull-up on, so ENABLE
// floats high and the motor runs at full speed. Call motor_init() as early
// as possible in main().

void motor_init(void);            // DIR low, ENABLE driven by PWM at 0 % duty -> motor off
void motor_set_speed(int8_t percent);   // -100..100: sign -> DIR, magnitude -> duty cycle
void motor_stop(void);            // duty cycle 0

#endif
