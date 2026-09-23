#ifndef OLED_H
#define OLED_H

#include <stdint.h>

// 128x64 SSD1309 on the IO-board. Pins are in spi.h; call spi_init() first.
void oled_init(void);
void oled_clear(void);
void oled_goto(uint8_t page, uint8_t col);   /* page 0-7, col 0-127 */
void oled_putchar(char c);                   /* A-Z, 0-9 and space  */
void oled_print(const char *s);

/* Draw a PROGMEM bitmap at column x, starting on page (0-7).
   width in pixels, height a multiple of 8. */
void oled_draw_image(uint8_t x, uint8_t page, uint8_t width, uint8_t height,
                     const uint8_t *img);   /* vertical byte format   */
void oled_draw_image_h(uint8_t x, uint8_t page, uint8_t width, uint8_t height,
                       const uint8_t *img); /* horizontal byte format */

/* Draws image page p (8 pixel rows) of a horizontal-format image on display
   page page. Only the rows whose bit is set in mask are shown, bit 0 = top. */
void oled_draw_image_h_page(uint8_t x, uint8_t page, uint8_t width,
                            const uint8_t *img, uint8_t p, uint8_t mask);

#endif
