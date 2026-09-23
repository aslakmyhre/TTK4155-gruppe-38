#ifndef OLED_H
#define OLED_H

#include <stdint.h>

/* 128x64 SSD1309 on the user IO-board; call spi_init() first. */
void oled_init(void);
/* 0/1: complementary checkerboards; 2: border and diagonal lines. */
void oled_test_pattern(uint8_t pattern);

#endif
