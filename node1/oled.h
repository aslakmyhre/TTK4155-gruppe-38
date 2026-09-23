#ifndef OLED_H
#define OLED_H

#include <stdint.h>

/* Pins on PORTB - change to match your wiring */
#define DISP_CS  PB0
#define DISP_DC  PB1
#define IO_CS    PB2

#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define OLED_PAGES  8

/* Direct display access */
void oled_init(void);
void oled_cmd(uint8_t c);
void oled_data(uint8_t d);
void oled_goto(uint8_t page, uint8_t col);
void oled_fill(uint8_t pattern);
void oled_clear(void);

/* Drawing goes to a framebuffer in external SRAM (requires xmem_init()).
   Nothing shows on the display until oled_flush() is called. */
void oled_fb_clear(void);
void oled_pixel(uint8_t x, uint8_t y);
void oled_rect(uint8_t x0, uint8_t y0, uint8_t w, uint8_t h);
void oled_fill_rect(uint8_t x0, uint8_t y0, uint8_t w, uint8_t h);
/* Draws text on one page (8-pixel row). Supports 0-9, '-', 'X', 'Y' and space. */
void oled_text(uint8_t page, uint8_t x, const char *s);
void oled_flush(void);

#endif