#include <stdio.h>
#include <util/delay.h>
#include "io_board.h"
#include "oled.h"
#include "spi.h"
#include "uart.h"

#define IO_BOARD_STARTUP_MS 500   /* lets the IO-board firmware boot after power-up */
#define REFRESH_MS          100

/* Fixed width, so a new value always overwrites every digit of the old one.
   The leading space avoids a trailing one: a 22nd character on a row would
   wrap around and overwrite the first. */
static void print_value(const char *label, uint8_t value)
{
    char digits[] = {'0' + value / 100, '0' + value / 10 % 10, '0' + value % 10, '\0'};
    oled_putchar(' ');
    oled_print(label);
    oled_putchar(' ');
    oled_print(digits);
}

static void draw_inputs(void)
{
    struct io_joystick joy = io_board_joystick();
    struct io_touchpad pad = io_board_touchpad();
    struct io_slider slider = io_board_slider();
    struct io_buttons buttons = io_board_buttons();

    oled_goto(0, 0);
    oled_print("JOY");
    print_value("X", joy.x);
    print_value("Y", joy.y);
    print_value("B", joy.button);

    oled_goto(2, 0);
    oled_print("PAD");
    print_value("X", pad.x);
    print_value("Y", pad.y);
    print_value("S", pad.size);

    oled_goto(4, 0);
    oled_print("SLIDER");
    print_value("X", slider.x);
    print_value("S", slider.size);

    oled_goto(6, 0);
    oled_print("BTN");
    print_value("R", buttons.right);
    print_value("L", buttons.left);
    print_value("N", buttons.nav);
}

int main(void)
{
    uart_init(UBRR_VALUE(9600));
    spi_init();
    oled_init();
    _delay_ms(IO_BOARD_STARTUP_MS);

    while (1) {
        draw_inputs();
        /* The OLED shares the failed SPI bus, so the error can only go to UART */
        if (spi_failed()) {
            printf("SPI stopped: SPCR=%02X SPSR=%02X DDRB=%02X PORTB=%02X PINB=%02X\n",
                   (unsigned int)SPCR, (unsigned int)SPSR,
                   (unsigned int)DDRB, (unsigned int)PORTB, (unsigned int)PINB);
            printf("Check SPE/MSTR and hardware SS (PB4). Reset after fixing.\n");
            while (1) {}
        }
        _delay_ms(REFRESH_MS);
    }
}
