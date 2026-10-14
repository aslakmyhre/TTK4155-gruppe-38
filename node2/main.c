#include <stdio.h>
#include <stdarg.h>

#include "sam.h"
#include "uart.h"
#include "can.h"

#define PB13_MASK (1 << 13)
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

// SysTick Interrupt Handler (Called every 1 ms)
void SysTick_Handler(void) {
    msTicks++;
}

// Initialize SysTick to trigger every 1 millisecond
void SysTick_Init(void) {
    // SystemCoreClock is typically 84000000 Hz on SAM3X
    SysTick_Config(SystemCoreClock / 1000); 
}

// Delay function in milliseconds
void delay_ms(volatile uint32_t ms) {
    uint32_t startTicks = msTicks;
    while ((msTicks - startTicks) < ms) {
        __WFI(); // Wait For Interrupt (saves power while waiting)
    }
}

int main()
{
    SystemInit();
    SysTick_Init();
    WDT->WDT_MR = WDT_MR_WDDIS; //Disable Watchdog Timer

    //Uncomment after including uart above
    uart_init(SystemCoreClock, 115200);   // SystemInit() runs the core at 84 MHz
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("Hello World\n\r");

    // Exercise 6.1: servo header signal pin (PB13) high
    PMC -> PMC_PCER0 = (1 << ID_PIOB);
    PIOB -> PIO_PER = PB13_MASK;
    PIOB->PIO_CODR = PB13_MASK;
    PIOB -> PIO_OER = PB13_MASK; 
    PIOB->PIO_SODR = PB13_MASK;    
    
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

        // exercise 6.3
        //printf("PB13 (servo signal) set high\n\r");
        /* code */
        //if (PIOB->PIO_ODSR & PB13_MASK)
        //    PIOB->PIO_CODR = PB13_MASK;
        //else 
        //    PIOB->PIO_SODR = PB13_MASK;
        //delay_ms(500);
        
    }
}