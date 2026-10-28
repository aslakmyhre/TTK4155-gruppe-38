#include "sam.h"
#include "motor.h"

#define MOTOR_EN_MASK   PIO_PB12    // ENABLE/PWM, D20
#define MOTOR_DIR_MASK  PIO_PC23    // PHASE/DIR, D7

// MCK = 84 MHz, no prescaler -> 84 MHz / 4200 = 20 kHz (driver needs < 50 kHz)
#define MOTOR_PWM_PERIOD   4200
#define MOTOR_MAX_PERCENT  100      // lower this to limit top speed
#define MOTOR_INVERT       0        // set to 1 if the motor runs the wrong way

void motor_init(void)
{
    PMC->PMC_PCER0 = (1u << ID_PIOB) | (1u << ID_PIOC);
    PMC->PMC_PCER1 = (1u << (ID_PWM - 32));

    // ENABLE: hold low as GPIO until the PWM channel is ready
    PIOB->PIO_CODR = MOTOR_EN_MASK;
    PIOB->PIO_PER  = MOTOR_EN_MASK;
    PIOB->PIO_OER  = MOTOR_EN_MASK;
    PIOB->PIO_PUDR = MOTOR_EN_MASK;

    // DIR: GPIO output, low
    PIOC->PIO_CODR = MOTOR_DIR_MASK;
    PIOC->PIO_PER  = MOTOR_DIR_MASK;
    PIOC->PIO_OER  = MOTOR_DIR_MASK;
    PIOC->PIO_PUDR = MOTOR_DIR_MASK;

    // Channel 0: CPOL = 1 so CDTY is the high time, same as the servo channel
    PWM->PWM_CH_NUM[0].PWM_CMR  = PWM_CMR_CPRE_MCK | PWM_CMR_CPOL;
    PWM->PWM_CH_NUM[0].PWM_CPRD = PWM_CPRD_CPRD(MOTOR_PWM_PERIOD);
    PWM->PWM_CH_NUM[0].PWM_CDTY = PWM_CDTY_CDTY(0);
    PWM->PWM_ENA = PWM_ENA_CHID0;

    // Now hand PB12 to the PWM module (peripheral B, PWMH0)
    PIOB->PIO_ABSR |= MOTOR_EN_MASK;
    PIOB->PIO_PDR   = MOTOR_EN_MASK;
}

void motor_set_speed(int8_t percent)
{
    if (percent > MOTOR_MAX_PERCENT)  percent = MOTOR_MAX_PERCENT;
    if (percent < -MOTOR_MAX_PERCENT) percent = -MOTOR_MAX_PERCENT;

#if MOTOR_INVERT
    percent = -percent;
#endif

    if (percent >= 0) {
        PIOC->PIO_SODR = MOTOR_DIR_MASK;
    } else {
        PIOC->PIO_CODR = MOTOR_DIR_MASK;
        percent = -percent;
    }

    uint32_t duty = (uint32_t)percent * MOTOR_PWM_PERIOD / 100;
    PWM->PWM_CH_NUM[0].PWM_CDTYUPD = PWM_CDTYUPD_CDTYUPD(duty);
}

void motor_stop(void)
{
    PWM->PWM_CH_NUM[0].PWM_CDTYUPD = PWM_CDTYUPD_CDTYUPD(0);
}
