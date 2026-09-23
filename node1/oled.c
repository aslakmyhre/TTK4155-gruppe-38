#include <avr/io.h>
#include "oled.h"

static void spi_init(void)
{
    /* MOSI (PB5), SCK (PB7) and CS/DC as outputs.
       PB4 (SS) must be an output to stay SPI master. */
    DDRB  |= (1 << PB5) | (1 << PB7) | (1 << PB4)
           | (1 << DISP_CS) | (1 << DISP_DC) | (1 << IO_CS);
    PORTB |= (1 << DISP_CS) | (1 << IO_CS);        /* deselect both */
    SPCR   = (1 << SPE) | (1 << MSTR) | (1 << SPR0); /* master, fosc/16 */
}

static void spi_transfer(uint8_t b)
{
    SPDR = b;
    while (!(SPSR & (1 << SPIF)));
}

void oled_cmd(uint8_t c)
{
    PORTB &= ~(1 << DISP_DC);   /* command mode */
    PORTB &= ~(1 << DISP_CS);
    spi_transfer(c);
    PORTB |=  (1 << DISP_CS);
}

void oled_data(uint8_t d)
{
    PORTB |=  (1 << DISP_DC);   /* data mode */
    PORTB &= ~(1 << DISP_CS);
    spi_transfer(d);
    PORTB |=  (1 << DISP_CS);
}

void oled_goto(uint8_t page, uint8_t col)
{
    oled_cmd(0xB0 | (page & 0x07));   /* page 0-7          */
    oled_cmd(0x00 | (col & 0x0F));    /* column low nibble */
    oled_cmd(0x10 | (col >> 4));      /* column high nibble */
}

void oled_fill(uint8_t pattern)
{
    for (uint8_t p = 0; p < 8; p++) {
        oled_goto(p, 0);
        for (uint8_t c = 0; c < 128; c++)
            oled_data(pattern);
    }
}

void oled_clear(void)
{
    oled_fill(0x00);
}

void oled_init(void)
{
    spi_init();
    oled_cmd(0xA1);   /* flip horizontally */
    oled_cmd(0xC8);   /* flip vertically   */
    oled_clear();
    oled_cmd(0xAF);   /* display on        */
}