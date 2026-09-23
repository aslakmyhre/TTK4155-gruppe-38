#ifndef OLED_H
#define OLED_H

#include <stdint.h>

// Display pins
#define DISP_DC  PB0
#define DISP_CS  PB1
#define IO_CS    PB2


// Display functions
void oled_init(void);
void oled_clear(void);
void oled_goto(uint8_t page, uint8_t col);   /* page 0-7, col 0-127 */
void oled_putchar(char c);                   /* A-Z, 0-9 and space  */
void oled_print(const char *s);

#endif