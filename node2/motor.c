#include "sam.h"
#include "motor.h"

#define MOTOR_EN_MASK   PIO_PB12    // ENABLE/PWM, D20
#define MOTOR_DIR_MASK  PIO_PC23    // PHASE/DIR, D7

void motor_init(void)
{
    PMC->PMC_PCER0 = (1u << ID_PIOB) | (1u << ID_PIOC);

    // ENABLE: GPIO output, low. Set the level before enabling the output
    // so the pin never drives high for even a moment.
    PIOB->PIO_CODR = MOTOR_EN_MASK;
    PIOB->PIO_PER  = MOTOR_EN_MASK;
    PIOB->PIO_OER  = MOTOR_EN_MASK;
    PIOB->PIO_PUDR = MOTOR_EN_MASK;   // pull-up off, it is what made ENABLE high

    // DIR: GPIO output, low (direction does not matter while ENABLE is low)
    PIOC->PIO_CODR = MOTOR_DIR_MASK;
    PIOC->PIO_PER  = MOTOR_DIR_MASK;
    PIOC->PIO_OER  = MOTOR_DIR_MASK;
    PIOC->PIO_PUDR = MOTOR_DIR_MASK;
}

void motor_stop(void)
{
    PIOB->PIO_CODR = MOTOR_EN_MASK;
}
