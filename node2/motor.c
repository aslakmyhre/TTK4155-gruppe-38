#include "sam.h"
#include "motor.h"

#define MOTOR_EN_MASK   PIO_PB12    // ENABLE/PWM, D20
#define MOTOR_DIR_MASK  PIO_PC23    // PHASE/DIR, D7

#define MOTOR_PWM_CH    0           // PB12 is PWMH0
#define MOTOR_PWM_CPRD  4200UL      // MCK / 4200 = 20 kHz, above hearing range
#define MOTOR_MAX_PCT   10
#define MOTOR_INVERT    1

void motor_init(void)
{
    PMC->PMC_PCER0 = (1u << ID_PIOB) | (1u << ID_PIOC);
    PMC->PMC_PCER1 = (1u << (ID_PWM - 32));

    // ENABLE: GPIO output, low. Set the level before enabling the output
    // so the pin never drives high for even a moment.
    PIOB->PIO_CODR = MOTOR_EN_MASK;
    PIOB->PIO_PER  = MOTOR_EN_MASK;
    PIOB->PIO_OER  = MOTOR_EN_MASK;
    PIOB->PIO_PUDR = MOTOR_EN_MASK;   // pull-up off, it is what made ENABLE high

    // DIR: GPIO output, low
    PIOC->PIO_CODR = MOTOR_DIR_MASK;
    PIOC->PIO_PER  = MOTOR_DIR_MASK;
    PIOC->PIO_OER  = MOTOR_DIR_MASK;
    PIOC->PIO_PUDR = MOTOR_DIR_MASK;

    // PWM channel 0 running at 0 % duty before PB12 is handed over, so the
    // pin stays low through the switch. CPOL as in pwm.c: high time = CDTY.
    PWM->PWM_CH_NUM[MOTOR_PWM_CH].PWM_CMR  = PWM_CMR_CPRE_MCK | PWM_CMR_CPOL;
    PWM->PWM_CH_NUM[MOTOR_PWM_CH].PWM_CPRD = PWM_CPRD_CPRD(MOTOR_PWM_CPRD);
    PWM->PWM_CH_NUM[MOTOR_PWM_CH].PWM_CDTY = PWM_CDTY_CDTY(0);
    PWM->PWM_ENA = PWM_ENA_CHID0;

    PIOB->PIO_ABSR |= MOTOR_EN_MASK;  // peripheral B = PWMH0
    PIOB->PIO_PDR   = MOTOR_EN_MASK;
}

void motor_set_speed(int8_t percent)
{
    if (percent > MOTOR_MAX_PCT)  percent = MOTOR_MAX_PCT;
    if (percent < -MOTOR_MAX_PCT) percent = -MOTOR_MAX_PCT;

#if MOTOR_INVERT
    percent =-percent;
#endif

    // Which DIR level is "right" depends on motor wiring: flip here if reversed
    if (percent < 0) {
        PIOC->PIO_CODR = MOTOR_DIR_MASK;
        percent = -percent;
    } else {
        PIOC->PIO_SODR = MOTOR_DIR_MASK;
    }

    uint32_t duty = (MOTOR_PWM_CPRD * (uint32_t)percent) / MOTOR_MAX_PCT;
    PWM->PWM_CH_NUM[MOTOR_PWM_CH].PWM_CDTYUPD = PWM_CDTYUPD_CDTYUPD(duty);
}

void motor_stop(void)
{
    PWM->PWM_CH_NUM[MOTOR_PWM_CH].PWM_CDTYUPD = PWM_CDTYUPD_CDTYUPD(0);
}
