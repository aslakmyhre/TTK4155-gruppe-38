#include <stdio.h>
#include <util/delay.h>
#include "io_board.h"
#include "spi.h"
#include "uart.h"

int main(void)
{
    uart_init(UBRR_VALUE(9600));
    spi_init();

    /* Observe the OLED before sending the first AVR command. */
    printf("\nJoystick-only test: waiting 5 seconds before first request.\n");
    _delay_ms(5000);
    printf("Starting command 0x03 reads; UART 9600 8N1.\n");

    while (1) {
        struct io_joystick joy = io_board_joystick();
        if (spi_failed()) {
            printf("SPI stopped: SPCR=%02X SPSR=%02X DDRB=%02X PORTB=%02X PINB=%02X\n",
                   (unsigned int)SPCR, (unsigned int)SPSR,
                   (unsigned int)DDRB, (unsigned int)PORTB, (unsigned int)PINB);
            printf("Check SPE/MSTR and hardware SS (PB4). Reset after fixing.\n");
            while (1) {}
        }
        printf("Joystick: X=%u Y=%u button=0x%02X\n",
               (unsigned int)joy.x, (unsigned int)joy.y,
               (unsigned int)joy.button);
        _delay_ms(500);
    }
}
