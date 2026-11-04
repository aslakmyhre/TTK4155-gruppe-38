#ifndef SOLENOID_H
#define SOLENOID_H

#include <stdint.h>

// Solenoid trigger on PB25 (D2), plain GPIO output.
// solenoid_fire() starts a short pulse, solenoid_update() ends it, so nothing blocks.

void solenoid_init(void);                   // pin inactive -> plunger retracted
void solenoid_fire(uint32_t now_ms);        // ignored while a pulse or cooldown is running
void solenoid_update(uint32_t now_ms);      // call every pass of the main loop

#endif
