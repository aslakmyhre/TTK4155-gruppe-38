#include <util/delay.h>
#include "io_board.h"
#include "spi.h"

static void begin(uint8_t command)
{
    spi_deselect_all();
    PORTB &= (uint8_t)~_BV(SPI_IO_CS);
    (void)spi_transfer(command);
    _delay_us(40);
}

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

static void write_led(uint8_t command, uint8_t led, uint8_t value)
{
    if (led >= IO_BOARD_LED_COUNT) {
        return;
    }
    begin(command);
    (void)spi_transfer(led);
    (void)spi_transfer(value);
    spi_deselect_all();
}

struct io_touchpad io_board_touchpad(void)
{
    uint8_t data[3];
    read_command(0x01, data, sizeof data);
    return (struct io_touchpad){data[0], data[1], data[2]};
}

struct io_slider io_board_slider(void)
{
    uint8_t data[2];
    read_command(0x02, data, sizeof data);
    return (struct io_slider){data[0], data[1]};
}

struct io_joystick io_board_joystick(void)
{
    uint8_t data[3];
    read_command(0x03, data, sizeof data);
    return (struct io_joystick){data[0], data[1], data[2]};
}

struct io_buttons io_board_buttons(void)
{
    uint8_t data[3];
    read_command(0x04, data, sizeof data);
    return (struct io_buttons){data[0], data[1], data[2]};
}

void io_board_led(uint8_t led, bool on)
{
    write_led(0x05, led, on ? 1 : 0);
}

void io_board_led_pwm(uint8_t led, uint8_t width)
{
    write_led(0x06, led, width);
}

void io_board_info(struct io_info *info)
{
    uint8_t data[35];
    read_command(0x07, data, sizeof data);
    for (uint8_t i = 0; i < 19; ++i) {
        info->timestamp[i] = (char)data[i];
    }
    info->timestamp[19] = '\0';
    for (uint8_t i = 0; i < sizeof info->serial; ++i) {
        info->serial[i] = data[19 + i];
    }
}
