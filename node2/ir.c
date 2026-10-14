#include "sam.h"
#include "ir.h"

// ---- Tuning: nothing is measured yet. Print ir_raw()/ir_filtered() with the beam
// ---- clear and blocked, then set the thresholds between the two levels.

// Due analog pin -> ADC channel: A0=7 A1=6 A2=5 A3=4 A4=3 A5=2 A6=1 A7=0 A8=10 A9=11 A10=12 A11=13
#define IR_ADC_CHANNEL       7       // A0 (PA16)
#define IR_BLOCKED_IS_LOW    1       // 1: less light -> lower voltage. 0 if the op-amp stage inverts
#define IR_THRESHOLD_BLOCK   1000    // filtered value must pass this to become blocked (0..4095)
#define IR_THRESHOLD_CLEAR   1500    // and pass back over this to be clear again (hysteresis gap)
#define IR_SAMPLE_PERIOD_MS  2
#define IR_FILTER_SHIFT      3       // EMA weight 1/8; time constant ~ 2 ms * 8 = 16 ms
#define IR_GOAL_HOLDOFF_MS   1000    // ignore new goals this long after one (ball bouncing in the beam)

#if IR_BLOCKED_IS_LOW
#if IR_THRESHOLD_BLOCK >= IR_THRESHOLD_CLEAR
#error "IR_BLOCKED_IS_LOW=1 needs IR_THRESHOLD_BLOCK < IR_THRESHOLD_CLEAR"
#endif
#else
#if IR_THRESHOLD_BLOCK <= IR_THRESHOLD_CLEAR
#error "IR_BLOCKED_IS_LOW=0 needs IR_THRESHOLD_BLOCK > IR_THRESHOLD_CLEAR"
#endif
#endif

#define IR_ADC_PRESCAL        41     // ADCClock = MCK / ((41 + 1) * 2) = 1 MHz, well under the 22 MHz max
#define IR_INIT_TIMEOUT_LOOPS 100000 // first conversion takes ~100 us; this is several ms
#define IR_CHANNEL_MASK       (1u << IR_ADC_CHANNEL)

static uint32_t filter_acc;          // filtered value << IR_FILTER_SHIFT, keeps the fraction
static uint16_t raw;
static int      blocked;
static int      goal_seen;         // holdoff only applies after the first goal
static uint32_t last_goal_ms;
static uint32_t last_sample_ms;

static uint16_t adc_read(void)
{
    return (uint16_t)(ADC->ADC_CDR[IR_ADC_CHANNEL] & ADC_CDR_DATA_Msk);
}

int ir_init(void)
{
    PMC->PMC_PCER1 = (1u << (ID_ADC - 32));

    // Free run: the ADC converts continuously, so reading is just a register load.
    // Enabling the channel hands the pin to the ADC, no PIO setup needed.
    ADC->ADC_MR   = ADC_MR_PRESCAL(IR_ADC_PRESCAL) | ADC_MR_STARTUP_SUT64
                  | ADC_MR_TRANSFER(1) | ADC_MR_FREERUN_ON;
    ADC->ADC_CHER = IR_CHANNEL_MASK;
    ADC->ADC_CR   = ADC_CR_START;

    uint32_t loops = 0;
    while (!(ADC->ADC_ISR & IR_CHANNEL_MASK))
        if (++loops >= IR_INIT_TIMEOUT_LOOPS)
            return -1;

    raw = adc_read();
    filter_acc = (uint32_t)raw << IR_FILTER_SHIFT;
    // Start as blocked so a goal needs the beam to be seen clear first:
    // no phantom goal at power-up if the ball sits in the beam or thresholds are off.
    blocked = 1;
    goal_seen = 0;
    return 0;
}

static int past_block_threshold(uint16_t v)
{
#if IR_BLOCKED_IS_LOW
    return v < IR_THRESHOLD_BLOCK;
#else
    return v > IR_THRESHOLD_BLOCK;
#endif
}

static int past_clear_threshold(uint16_t v)
{
#if IR_BLOCKED_IS_LOW
    return v > IR_THRESHOLD_CLEAR;
#else
    return v < IR_THRESHOLD_CLEAR;
#endif
}

int ir_update(uint32_t now_ms)
{
    if (now_ms - last_sample_ms < IR_SAMPLE_PERIOD_MS)
        return 0;
    last_sample_ms = now_ms;

    raw = adc_read();
    filter_acc += raw - (filter_acc >> IR_FILTER_SHIFT);
    uint16_t f = ir_filtered();

    if (blocked) {
        if (past_clear_threshold(f))
            blocked = 0;
        return 0;
    }
    if (!past_block_threshold(f))
        return 0;

    blocked = 1;
    if (goal_seen && now_ms - last_goal_ms < IR_GOAL_HOLDOFF_MS)
        return 0;
    goal_seen = 1;
    last_goal_ms = now_ms;
    return 1;
}

int      ir_blocked(void)     { return blocked; }
uint16_t ir_raw(void)         { return raw; }
uint16_t ir_filtered(void)    { return (uint16_t)(filter_acc >> IR_FILTER_SHIFT); }
