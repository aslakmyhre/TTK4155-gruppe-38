#ifndef IR_H
#define IR_H

#include <stdint.h>

// IR beam (goal) driver, lab manual 3.7.7 items 8-9. Score counting is in main.c.
// Reads the photodiode circuit with the internal ADC, low-pass filters it,
// and turns it into blocked/clear with hysteresis. A clear -> blocked edge is a goal.
// Pin, polarity, thresholds and timing are all set at the top of ir.c.
//
// The ADC pin takes 0-3.3 V only. Never connect the 5 V photodiode side directly.

int ir_init(void);                 // 0 ok, -1 if the ADC never finished a conversion

// Call from the main loop as often as you like; it samples every IR_SAMPLE_PERIOD_MS.
// Returns 1 exactly once per goal, otherwise 0.
int ir_update(uint32_t now_ms);

int      ir_blocked(void);         // 1 while the filtered signal says the beam is blocked
uint16_t ir_raw(void);             // last ADC sample, 0..4095 (print this to find thresholds)
uint16_t ir_filtered(void);        // low-pass filtered value, same scale

#endif
