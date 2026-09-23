#ifndef OLED_H
#define OLED_H

#include <stdint.h>
#include <stdio.h>

#define OLED_LINES   8     /* 8-pixel pages */
#define OLED_COLUMNS 128

// 128x64 SSD1309 on the IO-board. Pins are in spi.h; call spi_init() first.
void oled_init(void);
void oled_reset(void);
void oled_clear(void);
void oled_clear_line(uint8_t line);          /* leaves the cursor at its start */
void oled_home(void);
void oled_goto_line(uint8_t line);           /* 0-7, keeps the column  */
void oled_goto_column(uint8_t column);       /* 0-127, keeps the line  */
void oled_pos(uint8_t line, uint8_t column);
void oled_contrast(uint8_t level);           /* 0-255 */

/* Printable ASCII; anything else is drawn as '?' so it is visible */
void oled_putchar(char c);
void oled_print(const char *s);

/* For fprintf/fprintf_P. '\n' moves to the start of the next line. */
FILE *oled_output(void);

/* Draws image page p (8 pixel rows) of a PROGMEM bitmap in horizontal byte
   format (MSB = leftmost) at column x on display page (0-7). Only the rows
   whose bit is set in mask are shown, bit 0 = top. */
void oled_draw_image_h_page(uint8_t x, uint8_t page, uint8_t width,
                            const uint8_t *img, uint8_t p, uint8_t mask);

#endif
