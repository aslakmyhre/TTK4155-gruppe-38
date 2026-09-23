#include <stdio.h>
#include <util/delay.h>
#include "io_board.h"
#include "oled.h"
#include "spi.h"
#include "uart.h"

static void print_info(void)
{
    struct io_info info;
    io_board_info(&info);
    /* A plausible timestamp is a useful sanity check, not an SPI ACK. */
    uint8_t plausible = 1;
    for (uint8_t i = 0; i < 19; ++i) {
        char c = info.timestamp[i];
        char separator = (i == 4 || i == 7) ? '-'
                       : i == 10 ? 'T' : (i == 13 || i == 16) ? ':' : 0;
        if (separator ? c != separator : (c < '0' || c > '9')) {
            plausible = 0;
        }
    }
    if (plausible) {
        printf("Firmware: %s\n", info.timestamp);
    } else {
        printf("Unexpected timestamp bytes (check SPI wiring/mode/power):");
        for (uint8_t i = 0; i < 19; ++i) {
            printf(" %02X", (unsigned int)(uint8_t)info.timestamp[i]);
        }
        printf("\n");
    }
    printf("AVR serial:");
    for (uint8_t i = 0; i < sizeof info.serial; ++i) {
        printf(" %02X", (unsigned int)info.serial[i]);
    }
    printf("\n");
}

static void print_inputs(void)
{
    struct io_touchpad pad = io_board_touchpad();
    struct io_slider slider = io_board_slider();
    struct io_joystick joy = io_board_joystick();
    struct io_buttons buttons = io_board_buttons();
    printf("Pad X=%u Y=%u size=%u | Slider X=%u size=%u\n",
           (unsigned int)pad.x, (unsigned int)pad.y, (unsigned int)pad.size,
           (unsigned int)slider.x, (unsigned int)slider.size);
    printf("Joy X=%u Y=%u button=0x%02X | Buttons R=%02X L=%02X NAV=%02X\n",
           (unsigned int)joy.x, (unsigned int)joy.y, (unsigned int)joy.button,
           (unsigned int)buttons.right, (unsigned int)buttons.left,
           (unsigned int)buttons.nav);
}

int main(void)
{
    uart_init(UBRR_VALUE(9600));
    spi_init();
    _delay_ms(500); /* Allow board firmware to start after power-up. */
    printf("\nIO-board test: SPI mode 0, 38400 Hz; UART 9600 8N1\n");
    oled_init();

    while (1) {
        print_info();
        for (uint8_t led = 0; led < IO_BOARD_LED_COUNT; ++led) {
            io_board_led_pwm(led, 255);
            io_board_led(led, false);
        }

        /* Exercise command 0x05 on every LED, one at a time. */
        printf("LED on/off chase; OLED checkerboard\n");
        oled_test_pattern(0);
        for (uint8_t led = 0; led < IO_BOARD_LED_COUNT; ++led) {
            printf("LED %u ON\n", (unsigned int)led);
            io_board_led(led, true);
            print_inputs();
            _delay_ms(300);
            io_board_led(led, false);
        }

        /* Set on/off before PWM in case firmware keeps separate state. */
        printf("LED PWM sweep; OLED inverse checkerboard\n");
        oled_test_pattern(1);
        for (uint8_t led = 0; led < IO_BOARD_LED_COUNT; ++led) {
            io_board_led(led, true);
        }
        for (uint8_t step = 0; step <= 10; ++step) {
            uint8_t level = (step <= 5 ? step : 10 - step) * 51;
            printf("All LEDs PWM=%u\n", (unsigned int)level);
            for (uint8_t led = 0; led < IO_BOARD_LED_COUNT; ++led) {
                io_board_led_pwm(led, level);
            }
            print_inputs();
            _delay_ms(200);
        }
        for (uint8_t led = 0; led < IO_BOARD_LED_COUNT; ++led) {
            io_board_led(led, false);
        }

        printf("OLED border and diagonals; exercise all controls\n");
        oled_test_pattern(2);
        for (uint8_t sample = 0; sample < 10; ++sample) {
            print_inputs();
            _delay_ms(300);
        }
    }
}
