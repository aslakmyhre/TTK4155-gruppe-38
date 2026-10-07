#include <stdio.h>
#include <stdarg.h>

#include "sam.h"

#define PB13_MASK (1 << 13)

/*
 * Remember to update the Makefile with the (relative) path to the uart.c file.
 * This starter code will not compile until the UART file has been included in the Makefile. 
 * If you get somewhat cryptic errors referencing functions such as _sbrk, 
 * _close_r, _write_r, _fstat etc, you have most likely not done that correctly.

 * If you get errors such as "arm-none-eabi-gcc: no such file", you may need to reinstall the arm gcc packages using
 * apt or your favorite package manager.
 */
#include "uart.h"
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
    uart_init(84000000, 115200);   // SystemInit() runs the core at 84 MHz
    printf("Hello World\n\r");
    PMC -> PMC_PCER0 = (1 << ID_PIOB);

    PIOB -> PIO_PER = PB13_MASK;
    PIOB->PIO_CODR = PB13_MASK;
    PIOB -> PIO_OER = PB13_MASK; 

    // Exercise 6.1: servo header signal pin (PB13) high
    PIOB->PIO_SODR = PB13_MASK;
    printf("PB13 (servo signal) set high\n\r");

    while (1)
    {
        /* code */
        if (PIOB->PIO_ODSR & PB13_MASK)
            PIOB->PIO_CODR = PB13_MASK;
        else 
            PIOB->PIO_SODR = PB13_MASK;
        delay_ms(500);
    }
    
}