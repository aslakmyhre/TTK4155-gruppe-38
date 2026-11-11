#include "sam.h"
#include "solenoid.h"

#define SOLENOID_MASK         PIO_PB25    // D2
#define SOLENOID_ACTIVE_HIGH  0
#define SOLENOID_PULSE_MS     50
#define SOLENOID_COOLDOWN_MS  300

//1 while pulse is running
static int active = 0;
//time of last shot
static uint32_t fired_at = 0;

//set pin to active level, plunger out
static void pin_on(void)
{
#if SOLENOID_ACTIVE_HIGH
    PIOB->PIO_SODR = SOLENOID_MASK;
#else
    PIOB->PIO_CODR = SOLENOID_MASK;
#endif
}

//set pin to inactive level, plunger in
static void pin_off(void)
{
#if SOLENOID_ACTIVE_HIGH
    PIOB->PIO_CODR = SOLENOID_MASK;
#else
    PIOB->PIO_SODR = SOLENOID_MASK;
#endif
}

void solenoid_init(void)
{
    //enable clock for PIOB
    PMC->PMC_PCER0 = (1u << ID_PIOB);

    // Set the level before enabling the output, so it never fires at startup
    pin_off();
    PIOB->PIO_PER  = SOLENOID_MASK;
    PIOB->PIO_OER  = SOLENOID_MASK;
    PIOB->PIO_PUDR = SOLENOID_MASK;

    fired_at = 0;
    active = 0;
}

//start pulse, ignored while pulse or cooldown is running
void solenoid_fire(uint32_t now_ms)
{
    if (active || (now_ms - fired_at) < SOLENOID_COOLDOWN_MS)
        return;
    pin_on();
    fired_at = now_ms;
    active = 1;
}

//end pulse after SOLENOID_PULSE_MS
void solenoid_update(uint32_t now_ms)
{
    if (active && (now_ms - fired_at) >= SOLENOID_PULSE_MS) {
        pin_off();
        active = 0;
    }
}
