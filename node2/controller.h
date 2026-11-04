#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdint.h>

//PI position controller
//ref and pos are in encoder counts, error scaled to % of track
//output = motor speed % (-MOTOR_MAX_PCT, MOTOR_MAX_PCT)

void controller_init(int32_t pos_max);
void controller_reset(void);

#define CONTROLLER_PERIOD_MS 10
int8_t controller_update(int32_t ref, int32_t pos);

// Values from the latest controller_update(), all zero after a reset.
// p + i is the unclamped output, so p + i != u means the output is saturated.
typedef struct {
    int32_t ref;
    int32_t pos;
    float e;
    float p;
    float i;
    float u;
} ControllerState;

ControllerState controller_state(void);

#endif