#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

// Motor driver (phase/enable) on the motor shield.
//   ENABLE/PWM: PB12 (D20), PWM channel 0 (PWMH0), peripheral B, 20 kHz
//   PHASE/DIR:  PC23 (D7)
//
// Call motor_init() as early as possible in main(). After reset every SAM3X
// pin is an input with its pull-up on, so ENABLE floats high and the motor
// runs at full speed until this runs.

void motor_init(void);                  // PWM at 0 % duty -> motor off
void motor_set_speed(int8_t percent);   // -100..100, sign = direction
void motor_stop(void);                  // 0 % duty

#endif
