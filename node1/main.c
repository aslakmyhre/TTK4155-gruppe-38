#include <avr/pgmspace.h>
#include <stdbool.h>
#include <stdio.h>
#include <util/delay.h>
#include "adc.h"
#include "calibration.h"
#include "image.h"
#include "io_board.h"
#include "joystick.h"
#include "menu.h"
#include "oled.h"
#include "spi.h"
#include "uart.h"
#include "sram.h"
#include "xmem.h"

#define IO_BOARD_STARTUP_MS 500   /* lets the IO-board firmware boot after power-up */
#define SCREEN_PERIOD_MS    200
#define CONTRAST_BRIGHT     0xFF
#define CONTRAST_DIM        0x10
#define LED_FULL_BRIGHTNESS 255
#define EXIT_HINT_LINE      7
#define SPLASH_X            32    /* 64x64 image centered: (128 - 64) / 2 */
#define SPLASH_STEP_MS      15    /* pause per pixel row, sets the reveal speed */
#define SPLASH_HOLD_MS      500   /* finished image stays up before the menu */

#define COUNT(array) (sizeof (array) / sizeof (array)[0])

enum action {
    ACTION_INPUT_VIEW,
    ACTION_LED_TEST,
    ACTION_BRIGHT,
    ACTION_DIM,
};

static const struct menu_item settings_items[] = {
    { "BRIGHT", NULL, ACTION_BRIGHT },
    { "DIM",    NULL, ACTION_DIM },
};
static const struct menu settings_menu = { "SETTINGS", settings_items, COUNT(settings_items) };

static const struct menu_item main_items[] = {
    { "INPUT VIEW", NULL,           ACTION_INPUT_VIEW },
    { "LED TEST",   NULL,           ACTION_LED_TEST },
    { "SETTINGS",   &settings_menu, 0 },
};
static const struct menu main_menu = { "MAIN MENU", main_items, COUNT(main_items) };

static struct axis_calibration cal[ADC_NUM_CHANNELS];


/* True once per press. Start with *was_pressed = true when the screen was
   opened by a click, so the button has to be released first. */
static bool joystick_clicked(bool *was_pressed)
{
    bool pressed = io_board_joystick().button != 0;
    bool clicked = pressed && !*was_pressed;
    *was_pressed = pressed;
    return clicked;
}

static void print_adc_values(void)
{
    uint8_t raw[ADC_NUM_CHANNELS];
    uint8_t values[ADC_NUM_CHANNELS];
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
}

/* Fixed-width fields, so each redraw overwrites every digit of the last one */
static void draw_io_board_inputs(void)
{
    struct io_joystick joy = io_board_joystick();
    struct io_touchpad pad = io_board_touchpad();
    struct io_slider slider = io_board_slider();
    struct io_buttons buttons = io_board_buttons();
    FILE *out = oled_output();

    oled_pos(0, 0);
    fprintf_P(out, PSTR("JOY X %3u Y %3u B %3u"), joy.x, joy.y, joy.button);
    oled_pos(2, 0);
    fprintf_P(out, PSTR("PAD X %3u Y %3u S %3u"), pad.x, pad.y, pad.size);
    oled_pos(4, 0);
    fprintf_P(out, PSTR("SLIDER X %3u S %3u"), slider.x, slider.size);
    oled_pos(6, 0);
    fprintf_P(out, PSTR("BTN R %3u L %3u N %3u"), buttons.right, buttons.left, buttons.nav);
}

/* Live IO-board values on the OLED and calibrated ADC values on UART
   (for tools/plot_adc.py) until the joystick is clicked. */
static void show_inputs(void)
{
    bool was_pressed = true;
    oled_clear();
    oled_pos(EXIT_HINT_LINE, 0);
    oled_print("CLICK TO EXIT");
    while (!joystick_clicked(&was_pressed)) {
        print_adc_values();
        draw_io_board_inputs();
        _delay_ms(SCREEN_PERIOD_MS);
    }
}

/* Each right-hand button toggles the IO-board LED with the same bit number.
   The bit-to-button mapping is taken from the raw mask and is not verified. */
static void led_test(void)
{
    FILE *out = oled_output();
    uint8_t leds_on = 0;
    for (uint8_t led = 0; led < IO_BOARD_LED_COUNT; led++) {
        // Set PWM before on/off in case the firmware keeps them separately
        io_board_led_pwm(led, LED_FULL_BRIGHTNESS);
        io_board_led(led, false);
    }
    uint8_t previous = io_board_buttons().right;
    bool was_pressed = true;

    oled_clear();
    fprintf_P(out, PSTR("LED TEST\nRIGHT BUTTONS TOGGLE"));
    oled_pos(EXIT_HINT_LINE, 0);
    oled_print("CLICK TO EXIT");
    while (!joystick_clicked(&was_pressed)) {
        uint8_t right = io_board_buttons().right;
        uint8_t newly_pressed = right & ~previous;
        previous = right;
        for (uint8_t led = 0; led < IO_BOARD_LED_COUNT; led++) {
            if (newly_pressed & (1 << led)) {
                leds_on ^= 1 << led;
                io_board_led(led, leds_on & (1 << led));
            }
        }
        oled_pos(3, 0);
        fprintf_P(out, PSTR("BUTTONS %02X LEDS %02X"), right, leds_on);
        _delay_ms(SCREEN_PERIOD_MS);
    }
}

/* Reveals the image one pixel row at a time, from the bottom up. Only the
   page holding the new row changes, so only that page is redrawn. */
static void play_splash(void)
{
    oled_clear();
    for (int8_t row = IMAGE_HEIGHT - 1; row >= 0; row--) {
        uint8_t page = row / 8;
        uint8_t mask = 0xFF << (row % 8);   /* this row and the ones below it */
        oled_draw_image_h_page(SPLASH_X, page, IMAGE_WIDTH, image_bitmap, page, mask);
        _delay_ms(SPLASH_STEP_MS);
    }
    _delay_ms(SPLASH_HOLD_MS);
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

    // Stays on screen while calibration runs over UART
    play_splash();
    calibration_run(cal);

    while (1) {
        uint8_t action = menu_run(&main_menu, cal);
        switch (action) {
            case ACTION_INPUT_VIEW: show_inputs(); break;
            case ACTION_LED_TEST:   led_test(); break;
            case ACTION_BRIGHT:     oled_contrast(CONTRAST_BRIGHT); break;
            case ACTION_DIM:        oled_contrast(CONTRAST_DIM); break;
            default:
                printf_P(PSTR("main: menu returned unknown action %u\n"), action);
                break;
        }
        play_splash();
    }
}
