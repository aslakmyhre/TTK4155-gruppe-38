#ifndef OLED_H
#define OLED_H

#include <stdint.h>

/* Pins on PORTB - change to match your wiring */
#define DISP_CS  PB4
#define DISP_DC  PB2
#define IO_CS    PB3

void oled_init(void);
void oled_cmd(uint8_t c);
void oled_data(uint8_t d);
void oled_goto(uint8_t page, uint8_t col);
void oled_fill(uint8_t pattern);
void oled_clear(void);

#endif