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

#endif