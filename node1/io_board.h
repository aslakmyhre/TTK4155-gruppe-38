#ifndef IO_BOARD_H
#define IO_BOARD_H

#include <stdbool.h>
#include <stdint.h>

#define IO_BOARD_LED_COUNT 6

struct io_touchpad { uint8_t x, y, size; };
struct io_slider { uint8_t x, size; };
struct io_joystick { uint8_t x, y, button; };
/* Keep raw masks: PDF table and bitfield example disagree on nav L/D. */
struct io_buttons { uint8_t right, left, nav; };
struct io_info {
    char timestamp[20]; /* 19 received bytes plus our NUL terminator. */
    uint8_t serial[16];
};

/* Call spi_init() first. Blocking transactions must not be interleaved
 * with SPI operations from interrupts. */
struct io_touchpad io_board_touchpad(void);
struct io_slider io_board_slider(void);
struct io_joystick io_board_joystick(void);
struct io_buttons io_board_buttons(void);
void io_board_led(uint8_t led, bool on);
void io_board_led_pwm(uint8_t led, uint8_t width);
void io_board_info(struct io_info *info);

#endif
