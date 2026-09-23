#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include "adc.h"
#include "calibration.h"
#include "joystick.h"
#include "uart.h"
#include "sram.h"
#include "xmem.h"
#include "oled.h"
#include "input_display.h"


int main(void) {
    uart_init(UBRR_VALUE(9600));

    // PORTA/PORTC become the multiplexed address/data bus after this,
    // so no LED blinking on PA0 any more.
    xmem_init();
    adc_init();
    sram_test();
    oled_init();
    struct axis_calibration cal[ADC_NUM_CHANNELS];
    calibration_run(cal);

    uint8_t raw[ADC_NUM_CHANNELS];
    uint8_t values[ADC_NUM_CHANNELS];


    while (1) {

        oled_fill(0xFF);          /* all pixels on */
        _delay_ms(1000);

        oled_fill(0x0F);          /* horizontal stripes */
        _delay_ms(1000);

        oled_clear();             /* "HI" in the middle */
        oled_goto(3, 58);
        oled_data(0x7F); oled_data(0x08); oled_data(0x08);
        oled_data(0x08); oled_data(0x7F);
        oled_data(0x00);
        oled_data(0x41); oled_data(0x7F); oled_data(0x41);
        _delay_ms(1000);

        
        adc_read(raw);
        for (uint8_t channel = 0; channel < ADC_NUM_CHANNELS; channel++) {
            values[channel] = calibration_apply(&cal[channel], raw[channel]);
        }

        input_display_show(values);

        // Kept in the format tools/plot_adc.py parses
        printf("ADC: %3u %3u %3u %3u\n",
               values[0], values[1], values[2], values[3]);

        struct joystick_position pos = joystick_position(
            values[JOYSTICK_X_CHANNEL], values[JOYSTICK_Y_CHANNEL]);
        joystick_print_position(pos);
        // Raw values share this line: tools/plot_adc.py plots any line with
        // exactly four integers, so a separate raw line would be plotted too.
        printf("  %-7s  raw: %3u %3u %3u %3u\n",
               joystick_direction_name(joystick_direction(pos)),
               raw[0], raw[1], raw[2], raw[3]);
        _delay_ms(200);
    }
}
