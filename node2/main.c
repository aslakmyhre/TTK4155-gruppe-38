#include <stdio.h>
#include "sam.h"
#include "uart.h"
#include "can.h"
#include "pwm.h"                           // PWM driver (servo on PB13)

#define PB13_MASK (1u << 13)               // servo header signal pin (now owned by PWM driver)
#define CAN_DEBUG_PERIOD_MS 500

/*
 * Remember to update the Makefile with the (relative) path to the uart.c file.
 * This starter code will not compile until the UART file has been included in the Makefile. 
 * If you get somewhat cryptic errors referencing functions such as _sbrk, 
 * _close_r, _write_r, _fstat etc, you have most likely not done that correctly.

 * If you get errors such as "arm-none-eabi-gcc: no such file", you may need to reinstall the arm gcc packages using
 * apt or your favorite package manager.
 */

volatile uint32_t msTicks = 0;

// SysTick interrupt, every 1 ms
void SysTick_Handler(void) {
    msTicks++;
}

int main(void)
{
    SystemInit();                          // 84 MHz core clock
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

    // PWM on PB13 (servo header)
    pwm_init();
    printf("PWM CPRD = %lu (expect 13125)\n\r", PWM->PWM_CH_NUM[1].PWM_CPRD);

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

    // PWM test sequence: centre, min, max, then out-of-range values to test the clamp
    const uint32_t test_pulses[] = {1500, 900, 2100, 3000, 500};
    const uint32_t n_tests = sizeof(test_pulses) / sizeof(test_pulses[0]);
    uint32_t test_idx = 0;
    uint32_t lastStep = 0;

    while (1)
    {
        // Print every CAN message received 
        if (can_rx(&msg)){
            can_printmsg(msg);
        }

        // Bus debugging without blocking: one mailbox, so a delay here would drop frames.
        // CAN_SR bits 16-19: error active/warning/passive/bus off. CAN_ECR: REC bits 0-7, TEC bits 16-23
        if (msTicks - lastDebugPrint >= CAN_DEBUG_PERIOD_MS){
            lastDebugPrint = msTicks;
            printf("CAN_SR %08lX CAN_ECR %08lX\n\r", (unsigned long)CAN0->CAN_SR, (unsigned long)CAN0->CAN_ECR);
        }

        // PWM test: step to the next test pulse every 2 s
        if ((msTicks - lastStep) >= 2000) {
            lastStep = msTicks;
            uint32_t p = test_pulses[test_idx];
            pwm_set_pulse_us(p);
            printf("Requested %lu us (expect %lu us on scope)\n\r",
                   p, p < 900 ? 900 : (p > 2100 ? 2100 : p));
            test_idx = (test_idx + 1) % n_tests;
        }

        // exercise 6.3 (old PB13 heartbeat, disabled: PB13 is now PWM)
        //if (PIOB->PIO_ODSR & PB13_MASK)
        //    PIOB->PIO_CODR = PB13_MASK;
        //else 
        //    PIOB->PIO_SODR = PB13_MASK;
        //delay_ms(500);
    }
}