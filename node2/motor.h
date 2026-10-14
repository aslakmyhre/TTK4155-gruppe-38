#ifndef MOTOR_H
#define MOTOR_H

// Motor driver (A3959, phase/enable) on the motor shield.
//   ENABLE/PWM: PB12 (D20), later PWM channel 0, peripheral B
//   PHASE/DIR:  PC23 (D7)
//
// For now this only holds the motor OFF. After reset every SAM3X pin is an
// input with its pull-up on, so ENABLE floats high and the motor runs at full
// speed. Call motor_init() as early as possible in main().

void motor_init(void);   // ENABLE and DIR as outputs, both low -> motor off
void motor_stop(void);   // ENABLE low

#endif
