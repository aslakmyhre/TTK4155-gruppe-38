#include "sam.h"
#include "solenoid.h"

#define SOLENOID_MASK         PIO_PB25    // D2
#define SOLENOID_ACTIVE_HIGH  1           // set to 0 if the plunger is out right after reset
#define SOLENOID_PULSE_MS     50          // long enough to hit the ball, raise if it is too weak
#define SOLENOID_COOLDOWN_MS  300         // minimum time between two shots

static int active = 0;
static uint32_t fired_at = 0;

static void pin_on(void)
{
#if SOLENOID_ACTIVE_HIGH
    PIOB->PIO_SODR = SOLENOID_MASK;
#else
    PIOB->PIO_CODR = SOLENOID_MASK;
#endif
}

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
    PMC->PMC_PCER0 = (1u << ID_PIOB);

    // Set the level before enabling the output, so it never fires at startup
    pin_off();
    PIOB->PIO_PER  = SOLENOID_MASK;
    PIOB->PIO_OER  = SOLENOID_MASK;
    PIOB->PIO_PUDR = SOLENOID_MASK;

    fired_at = 0;
    active = 0;
}

void solenoid_fire(uint32_t now_ms)
{
    if (active || (now_ms - fired_at) < SOLENOID_COOLDOWN_MS)
        return;
    pin_on();
    fired_at = now_ms;
    active = 1;
}

void solenoid_update(uint32_t now_ms)
{
    if (active && (now_ms - fired_at) >= SOLENOID_PULSE_MS) {
        pin_off();
        active = 0;
    }
}
