#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <util/delay.h>
#include "adc.h"
#include "calibration.h"
#include "io_board.h"
#include "joystick.h"
#include "oled.h"
#include "spi.h"
#include "uart.h"
#include "sram.h"
#include "xmem.h"

#define IO_BOARD_STARTUP_MS 500   /* lets the IO-board firmware boot after power-up */


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

static void draw_io_board_inputs(void)
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

/* The OLED shares the failed SPI bus, so the error can only go to UART */
static void halt_on_spi_failure(void)
{
    if (!spi_failed())
        return;
    printf_P(PSTR("SPI stopped: SPCR=%02X SPSR=%02X DDRB=%02X PORTB=%02X PINB=%02X\n"),
             (unsigned int)SPCR, (unsigned int)SPSR,
             (unsigned int)DDRB, (unsigned int)PORTB, (unsigned int)PINB);
    printf_P(PSTR("Check SPE/MSTR and hardware SS (PB4). Reset after fixing.\n"));
    while (1) {}
}

int main(void) {
    uart_init(UBRR_VALUE(9600));

    // PORTA/PORTC become the multiplexed address/data bus after this,
    // so no LED blinking on PA0 any more.
    xmem_init();
    adc_init();
    sram_test();

    spi_init();
    oled_init();
    _delay_ms(IO_BOARD_STARTUP_MS);

    struct axis_calibration cal[ADC_NUM_CHANNELS];
    calibration_run(cal);

    uint8_t raw[ADC_NUM_CHANNELS];
    uint8_t values[ADC_NUM_CHANNELS];
    while (1) {
        adc_read(raw);
        for (uint8_t channel = 0; channel < ADC_NUM_CHANNELS; channel++) {
            values[channel] = calibration_apply(&cal[channel], raw[channel]);
        }
        // Kept in the format tools/plot_adc.py parses
        printf_P(PSTR("ADC: %3u %3u %3u %3u\n"),
                 values[0], values[1], values[2], values[3]);

        struct joystick_position pos = joystick_position(
            values[JOYSTICK_X_CHANNEL], values[JOYSTICK_Y_CHANNEL]);
        joystick_print_position(pos);
        // Raw values share this line: tools/plot_adc.py plots any line with
        // exactly four integers, so a separate raw line would be plotted too.
        printf_P(PSTR("  %-7S  raw: %3u %3u %3u %3u\n"),
                 joystick_direction_name(joystick_direction(pos)),
                 raw[0], raw[1], raw[2], raw[3]);

        draw_io_board_inputs();
        halt_on_spi_failure();
        _delay_ms(200);
    }
}
