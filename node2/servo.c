#include "servo.h"
#include "pwm.h"

#define SERVO_CENTER_US      1500
#define SERVO_US_PER_PERCENT 6      // 100 % * 6 us = 600 us -> 900..2100 us
#define SERVO_INVERT         0      // set to 1 if the servo moves the wrong way

void servo_init(void)
{
    pwm_init();                     // starts at 1.5 ms (centre)
}

void servo_set_position(int8_t percent)
{
    // Safety layer 1: keep the input inside -100..100
    if (percent > 100)  percent = 100;
    if (percent < -100) percent = -100;

#if SERVO_INVERT
    percent = -percent;
#endif

    // -100 -> 900 us, 0 -> 1500 us, 100 -> 2100 us
    int32_t pulse_us = SERVO_CENTER_US + (int32_t)percent * SERVO_US_PER_PERCENT;

    // Safety layer 2: pwm_set_pulse_us() clamps to 900..2100 us as well
    pwm_set_pulse_us((uint32_t)pulse_us);
}
