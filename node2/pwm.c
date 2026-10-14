#include "sam.h"
#include "pwm.h"

// MCK = 84 MHz, divided by 128 -> 656 250 Hz PWM clock 
#define PWM_CLK_HZ      656250UL    // PWM counter ticks
#define PERIOD_US       20000UL     // 20 ms -> 50 Hz (Full PWM period)
#define PULSE_MIN_US    900UL       // Shortest pulse
#define PULSE_MAX_US    2100UL      // Longest pulse
#define PULSE_CENTER_US 1500UL      // Servo's centre position


static uint32_t us_to_ticks(uint32_t us){
    // 64-bit maths: 20000 * 656250 = 1.3e10 does not fit in 32 bits
    return (uint32_t)(((uint64_t)us * PWM_CLK_HZ) / 1000000UL); // 20000 us -> 13125 ticks
}

void pwm_init(void)
{
    // PIOB (ID 12): PCER0, PWM (ID 36): PCER1
    PMC->PMC_PCER0 = (1u << ID_PIOB);
    PMC->PMC_PCER1 = (1u << (ID_PWM - 32));

    // PIO releases PB13, Peripheral B (PWMH1) occupies it
    PIOB->PIO_PDR   = PIO_PDR_P13;
    PIOB->PIO_ABSR |= PIO_ABSR_P13;
    
    // CHannel 1 setup
    PWM->PWM_CH_NUM[1].PWM_CMR  = PWM_CMR_CPRE_MCK_DIV_128   // clock = MCK/128
                                | PWM_CMR_CPOL;              // polarity: check on scope!
    PWM->PWM_CH_NUM[1].PWM_CPRD = PWM_CPRD_CPRD(us_to_ticks(PERIOD_US));
    PWM->PWM_CH_NUM[1].PWM_CDTY = PWM_CDTY_CDTY(us_to_ticks(PULSE_CENTER_US));
    
    // Enable channel 1
    PWM->PWM_ENA = PWM_ENA_CHID1;
}

void pwm_set_pulse_us(uint32_t pulse_us)
{
    // Safety clamp
    if (pulse_us < PULSE_MIN_US) pulse_us = PULSE_MIN_US;
    if (pulse_us > PULSE_MAX_US) pulse_us = PULSE_MAX_US;

    // Update duty cycle
    PWM->PWM_CH_NUM[1].PWM_CDTYUPD = PWM_CDTYUPD_CDTYUPD(us_to_ticks(pulse_us));
}
