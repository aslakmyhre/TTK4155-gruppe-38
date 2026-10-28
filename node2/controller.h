#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdint.h>

// PI position controller for the motor.
// Reference and position are in encoder counts, the error is scaled to
// percent of the track so the gains do not depend on the encoder resolution.
// Output is motor speed in percent, -100..100 (motor_set_speed() scale).

void controller_init(int32_t pos_max);      // pos_max = track length in counts
void controller_reset(void);                // clear the integral

// Call at a fixed rate, every CONTROLLER_PERIOD_MS
#define CONTROLLER_PERIOD_MS 10
int8_t controller_update(int32_t ref, int32_t pos);

#endif
