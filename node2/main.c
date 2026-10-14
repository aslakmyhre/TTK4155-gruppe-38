#include <stdio.h>
#include "sam.h"
#include "uart.h"
#include "can.h"
#include "pwm.h"                           // NEW: PWM driver

// #define PB13_MASK (1u << 13)   // servo header signal pin (now owned by PWM driver)

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

    // PB13 as output (Exercise 6.1). Disabled: PB13 is now driven by PWM
    // PMC->PMC_PCER0 = (1u << ID_PIOB);
    // PIOB->PIO_PER  = PB13_MASK;
    // PIOB->PIO_OER  = PB13_MASK;
    // PIOB->PIO_SODR = PB13_MASK;

    // NEW: PWM on PB13 (servo header)
    pwm_init();
    printf("PWM CPRD = %lu (expect 13125)\n\r", PWM->PWM_CH_NUM[1].PWM_CPRD);

    // CAN0: 125 kbit/s, TQ = 500 ns, 16 TQ per bit, sample point 75 %
    // Must match MCP2515 on Node 1: CNF1 = 0x03, CNF2 = 0xAC, CNF3 = 0x03
    can_init((CanInit){
        .brp    = 41,   // TQ = (41+1) / 84 MHz = 500 ns
        .propag = 4,    // 5 TQ
        .phase1 = 5,    // 6 TQ
        .phase2 = 3,    // 4 TQ
        .sjw    = 0,    // 1 TQ
        .smp    = 0     // sample once
    }, 0);              // 0 = no RX interrupt, we poll
    printf("CAN_BR = 0x%08lX (expect 0x00290453)\n\r", CAN0->CAN_BR);

    // uint32_t lastToggle = 0;
    CanMsg msg;

    // NEW: PWM test sequence: centre, min, max, then out-of-range values to test the clamp
    const uint32_t test_pulses[] = {1500, 900, 2100, 3000, 500};
    const uint32_t n_tests = sizeof(test_pulses) / sizeof(test_pulses[0]);
    uint32_t test_idx = 0;
    uint32_t lastStep = 0;

    while (1)
    {
        // Print every CAN message received
        if (can_rx(&msg)) {
            can_printmsg(msg);
        }

        // Toggle PB13 every 500 ms without blocking (heartbeat). Disabled: PB13 is now PWM
        // if ((msTicks - lastToggle) >= 500) {
        //     lastToggle = msTicks;
        //     if (PIOB->PIO_ODSR & PB13_MASK)
        //         PIOB->PIO_CODR = PB13_MASK;
        //     else
        //         PIOB->PIO_SODR = PB13_MASK;
        // }

        // NEW: step to the next test pulse every 2 s
        if ((msTicks - lastStep) >= 2000) {
            lastStep = msTicks;
            uint32_t p = test_pulses[test_idx];
            pwm_set_pulse_us(p);
            printf("Requested %lu us (expect %lu us on scope)\n\r",
                   p, p < 900 ? 900 : (p > 2100 ? 2100 : p));
            test_idx = (test_idx + 1) % n_tests;
        }
    }
}