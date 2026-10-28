#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

// Motor encoder on TC2 channel 0 (TC6) in quadrature decoder mode.
//   Phase A: PC25 (D5), TIOA6, peripheral B
//   Phase B: PC26 (D4), TIOB6, peripheral B
// D4 is also wired to PA29 on the Due, leave PA29 as an input.

void encoder_init(void);
int32_t encoder_read(void);     // position in counts, signed
void encoder_reset(void);       // position -> 0

#endif
