#ifndef IO_BOARD_H
#define IO_BOARD_H

#include <stdbool.h>
#include <stdint.h>

#define IO_BOARD_LED_COUNT 6

#define IO_BOARD_TOUCH_PAD      0x01
#define IO_BOARD_TOUCH_SLIDER   0x02
#define IO_BOARD_JOYSTICK       0x03
#define IO_BOARD_BUTTONS        0x04
#define IO_BOARD_LED_ON_OFF     0x05
#define IO_BOARD_LED_PWM        0x06
#define IO_BOARD_INFO           0x07



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
