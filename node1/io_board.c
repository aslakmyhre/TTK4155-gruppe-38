/* Interface for communicating with the onboard MCU on the IO-board */

#include <util/delay.h>
#include "io_board.h"
#include "spi.h"


/* Standard for initiating transmission */
static void begin(uint8_t command)
{
    spi_select(SPI_SLAVE_IO);
    (void)spi_transfer(command);
    _delay_us(40);
}

/* Standard for reading from AVR */
static void read_command(uint8_t command, uint8_t *data, uint8_t count)
{
    begin(command);
    for (uint8_t i = 0; i < count; ++i) {
        if (i != 0) {
            _delay_us(2);
        }
        data[i] = spi_transfer(0x00);
    }
    spi_deselect_all();
}

/* Decide LED and index of LED */
static void write_led(uint8_t command, uint8_t led, uint8_t value)
{
    if (led >= IO_BOARD_LED_COUNT) {
        return;
    }
    const uint8_t data[] = {led, value};
    begin(command);
    spi_write(data, sizeof data);
    spi_deselect_all();
}

/* Get touchpad */
struct io_touchpad io_board_touchpad(void)
{
    uint8_t data[3];
    read_command(IO_BOARD_TOUCH_PAD, data, sizeof data);
    return (struct io_touchpad){data[0], data[1], data[2]};
}

/* Get slider*/
struct io_slider io_board_slider(void)
{
    uint8_t data[2];
    read_command(IO_BOARD_TOUCH_SLIDER, data, sizeof data);
    return (struct io_slider){data[0], data[1]}; // (X, Size)
}

/* Get joystick */
struct io_joystick io_board_joystick(void)
{
    uint8_t data[3];
    read_command(IO_BOARD_JOYSTICK, data, sizeof data);
    return (struct io_joystick){data[0], data[1], data[2]}; // (X,Y,btn)
}

/* Get button press */
struct io_buttons io_board_buttons(void)
{
    uint8_t data[3];
    read_command(IO_BOARD_BUTTONS, data, sizeof data);
    return (struct io_buttons){data[0], data[1], data[2]}; // (right left nav)
}

/* LED on/off */
void io_board_led(uint8_t led, bool on)
{
    write_led(IO_BOARD_LED_ON_OFF, led, on ? 1 : 0);
}

/* Write PWM width */
void io_board_led_pwm(uint8_t led, uint8_t width)
{
    write_led(IO_BOARD_LED_PWM, led, width);
}

/* Updates infor struct */
void io_board_info(struct io_info *info)
{
    uint8_t data[35];
    read_command(IO_BOARD_INFO, data, sizeof data);
    for (uint8_t i = 0; i < 19; ++i) {
        info->timestamp[i] = (char)data[i];
    }
    info->timestamp[19] = '\0';
    for (uint8_t i = 0; i < sizeof info->serial; ++i) {
        info->serial[i] = data[19 + i];
    }
}
