#include <stdio.h>
#include "sam.h"
#include "uart.h"
#include "can.h"
#include "pwm.h"                           // PWM driver (servo on PB13)
#include "servo.h"                         // Servo driver: percent -> pulse width
#include "motor.h"                         // Motor driver: held off for now
#include "ir.h"                            // IR beam: goal detection on the internal ADC

#define PB13_MASK (1u << 13)               // servo header signal pin (now owned by PWM driver)
#define CAN_DEBUG_PERIOD_MS 500

// Joystick frame from node 1 (node1/main.c, can_test):
//   id 0x010, length 2, byte[0] = x + 100, byte[1] = y + 100  (x, y in -100..100 percent)
#define JOYSTICK_CAN_ID      0x010
#define JOYSTICK_CAN_LENGTH  2
#define JOYSTICK_OFFSET      100
#define JOYSTICK_TIMEOUT_MS  500           // no joystick frame for this long -> servo to centre

/*
 * Remember to update the Makefile with the (relative) path to the uart.c file.
 * This starter code will not compile until the UART file has been included in the Makefile. 
 * If you get somewhat cryptic errors referencing functions such as _sbrk, 
 * _close_r, _write_r, _fstat etc, you have most likely not done that correctly.

 * If you get errors such as "arm-none-eabi-gcc: no such file", you may need to reinstall the arm gcc packages using
 * apt or your favorite package manager.
 */

volatile uint32_t msTicks = 0;
static uint32_t score = 0;                 // goals since start, read by the game later

// SysTick interrupt, every 1 ms
void SysTick_Handler(void) {
    msTicks++;
}

// Decodes a joystick frame into x and y percent. Returns 0 if the frame is not
// a valid joystick frame, so garbage on the bus can never move the servo.
static int joystick_decode(const CanMsg *m, int8_t *x, int8_t *y)
{
    if (m->id != JOYSTICK_CAN_ID || m->length != JOYSTICK_CAN_LENGTH)
        return 0;
    if (m->byte[0] > 2 * JOYSTICK_OFFSET || m->byte[1] > 2 * JOYSTICK_OFFSET)
        return 0;                          // outside 0..200, not a valid position
    *x = (int8_t)((int16_t)m->byte[0] - JOYSTICK_OFFSET);
    *y = (int8_t)((int16_t)m->byte[1] - JOYSTICK_OFFSET);
    return 1;
}

// Lab 3.7.7 item 10: one point each time the ball breaks the IR beam.
// ir_update() already filters noise and ignores bounces, so it reports each goal once.
static void score_count(uint32_t now_ms)
{
    if (!ir_update(now_ms))
        return;
    score++;
    printf("GOAL! score %lu\n\r", (unsigned long)score);
}

int main(void)
{
    SystemInit();                          // 84 MHz core clock
    motor_init();                          // FIRST: stop the motor (ENABLE pin floats high after reset)
    WDT->WDT_MR = WDT_MR_WDDIS;            // disable watchdog
    SysTick_Config(SystemCoreClock / 1000);

    // UART over USB (programming port), printf goes here
    uart_init(SystemCoreClock, 115200);
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("Node 2 started, MCK = %lu Hz\n\r", SystemCoreClock);  // should print 84000000

    // Exercise 6.1: servo header signal pin (PB13) high.
    // Disabled: PB13 is now driven by the PWM module (Exercise 7)
    // PMC -> PMC_PCER0 = (1 << ID_PIOB);
    // PIOB -> PIO_PER = PB13_MASK;
    // PIOB->PIO_CODR = PB13_MASK;
    // PIOB -> PIO_OER = PB13_MASK; 
    // PIOB->PIO_SODR = PB13_MASK;    

    // PWM on PB13 (servo header), via the servo driver. Starts at centre (1.5 ms)
    // pwm_init();                         // replaced by servo_init(), which calls it
    servo_init();
    printf("PWM CPRD = %lu (expect 13125)\n\r", PWM->PWM_CH_NUM[1].PWM_CPRD);

    if (ir_init() != 0)
        printf("IR: ADC init failed, no conversion\n\r");

    // Must match node 1's MCP2515 CNF1-3 = 0x03, 0xAE, 0x01. Each field is TQ - 1:
    // TQ = 42 / 84 MHz = 0.5 us, sync 1 + prop 7 + phase1 6 + phase2 2 = 16 TQ = 125 kbit/s
    can_init((CanInit){
        .brp = 41,
        .propag = 6,
        .phase1 = 5,
        .phase2 = 1,
        .sjw = 0,
        .smp = 0
    }, 0);
    printf("CAN_BR = 0x%08lX (expect 0x00290651)\n\r", (unsigned long)CAN0->CAN_BR);

    CanMsg msg;
    uint32_t lastDebugPrint = msTicks;

    // Joystick state (servo follows the y-axis, the lab says x-axis is for the motor)
    int8_t joy_x = 0;
    int8_t joy_y = 0;
    uint32_t lastJoystick = msTicks;
    int joystick_timed_out = 1;            // centre until the first frame arrives

    // PWM test sequence (Exercise 7.1/7.3). Disabled: the servo now follows the joystick
    // const uint32_t test_pulses[] = {1500, 900, 2100, 3000, 500};
    // const uint32_t n_tests = sizeof(test_pulses) / sizeof(test_pulses[0]);
    // uint32_t test_idx = 0;
    // uint32_t lastStep = 0;

    while (1)
    {
        // Read every CAN frame; joystick frames move the servo
        if (can_rx(&msg)){
            if (joystick_decode(&msg, &joy_x, &joy_y)) {
                servo_set_position(joy_y);
                lastJoystick = msTicks;
                joystick_timed_out = 0;
            } else {
                can_printmsg(msg);         // anything else: print it so we notice
            }
        }

        score_count(msTicks);

        // Safety: if node 1 stops sending (cable out, left the CAN TEST screen), go to centre
        if (!joystick_timed_out && (msTicks - lastJoystick) >= JOYSTICK_TIMEOUT_MS) {
            joystick_timed_out = 1;
            servo_set_position(0);
            printf("Joystick timeout, servo to centre\n\r");
        }

        // Bus debugging without blocking: one mailbox, so a delay here would drop frames.
        // CAN_SR bits 16-19: error active/warning/passive/bus off. CAN_ECR: REC bits 0-7, TEC bits 16-23
        if (msTicks - lastDebugPrint >= CAN_DEBUG_PERIOD_MS){
            lastDebugPrint = msTicks;
            printf("CAN_SR %08lX CAN_ECR %08lX  joy x %4d y %4d  CDTY %lu  IR raw %4u filt %4u %s\n\r",
                   (unsigned long)CAN0->CAN_SR, (unsigned long)CAN0->CAN_ECR,
                   joy_x, joy_y, (unsigned long)PWM->PWM_CH_NUM[1].PWM_CDTY,
                   ir_raw(), ir_filtered(), ir_blocked() ? "blocked" : "clear");
        }

        // PWM test: step to the next test pulse every 2 s. Disabled: servo follows joystick
        // if ((msTicks - lastStep) >= 2000) {
        //     lastStep = msTicks;
        //     uint32_t p = test_pulses[test_idx];
        //     pwm_set_pulse_us(p);
        //     printf("Requested %lu us (expect %lu us on scope)\n\r",
        //            p, p < 900 ? 900 : (p > 2100 ? 2100 : p));
        //     test_idx = (test_idx + 1) % n_tests;
        // }

        // exercise 6.3 (old PB13 heartbeat, disabled: PB13 is now PWM)
        //if (PIOB->PIO_ODSR & PB13_MASK)
        //    PIOB->PIO_CODR = PB13_MASK;
        //else 
        //    PIOB->PIO_SODR = PB13_MASK;
        //delay_ms(500);
    }
}
