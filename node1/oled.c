#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "oled.h"

/* 5x7 font, one byte per column, bit 0 = top pixel. Stored in flash. */
static const uint8_t letters[26][5] PROGMEM = {
    {0x7E,0x11,0x11,0x11,0x7E}, /* A */
    {0x7F,0x49,0x49,0x49,0x36}, /* B */
    {0x3E,0x41,0x41,0x41,0x22}, /* C */
    {0x7F,0x41,0x41,0x22,0x1C}, /* D */
    {0x7F,0x49,0x49,0x49,0x41}, /* E */
    {0x7F,0x09,0x09,0x09,0x01}, /* F */
    {0x3E,0x41,0x49,0x49,0x7A}, /* G */
    {0x7F,0x08,0x08,0x08,0x7F}, /* H */
    {0x00,0x41,0x7F,0x41,0x00}, /* I */
    {0x20,0x40,0x41,0x3F,0x01}, /* J */
    {0x7F,0x08,0x14,0x22,0x41}, /* K */
    {0x7F,0x40,0x40,0x40,0x40}, /* L */
    {0x7F,0x02,0x0C,0x02,0x7F}, /* M */
    {0x7F,0x04,0x08,0x10,0x7F}, /* N */
    {0x3E,0x41,0x41,0x41,0x3E}, /* O */
    {0x7F,0x09,0x09,0x09,0x06}, /* P */
    {0x3E,0x41,0x51,0x21,0x5E}, /* Q */
    {0x7F,0x09,0x19,0x29,0x46}, /* R */
    {0x46,0x49,0x49,0x49,0x31}, /* S */
    {0x01,0x01,0x7F,0x01,0x01}, /* T */
    {0x3F,0x40,0x40,0x40,0x3F}, /* U */
    {0x1F,0x20,0x40,0x20,0x1F}, /* V */
    {0x3F,0x40,0x38,0x40,0x3F}, /* W */
    {0x63,0x14,0x08,0x14,0x63}, /* X */
    {0x07,0x08,0x70,0x08,0x07}, /* Y */
    {0x61,0x51,0x49,0x45,0x43}, /* Z */
};

static const uint8_t digits[10][5] PROGMEM = {
    {0x3E,0x51,0x49,0x45,0x3E}, /* 0 */
    {0x00,0x42,0x7F,0x40,0x00}, /* 1 */
    {0x42,0x61,0x51,0x49,0x46}, /* 2 */
    {0x21,0x41,0x45,0x4B,0x31}, /* 3 */
    {0x18,0x14,0x12,0x7F,0x10}, /* 4 */
    {0x27,0x45,0x45,0x45,0x39}, /* 5 */
    {0x3C,0x4A,0x49,0x49,0x30}, /* 6 */
    {0x01,0x71,0x09,0x05,0x03}, /* 7 */
    {0x36,0x49,0x49,0x49,0x36}, /* 8 */
    {0x06,0x49,0x49,0x29,0x1E}, /* 9 */
};

static const uint8_t blank[5] PROGMEM = {0};


static void spi_init(void)
{
    /* MOSI (PB5), SCK (PB7) and CS/DC as outputs.
       PB4 (SS) must be an output to stay SPI master. */
    DDRB  |= (1 << PB5) | (1 << PB7) | (1 << PB4)
           | (1 << DISP_CS) | (1 << DISP_DC) | (1 << IO_CS);
    PORTB |= (1 << DISP_CS) | (1 << IO_CS);          /* deselect both */
    SPCR   = (1 << SPE) | (1 << MSTR) | (1 << SPR0); /* master, fosc/16 */
}

static void spi_transfer(uint8_t b)
{
    SPDR = b;
    while (!(SPSR & (1 << SPIF)));
}

static void oled_cmd(uint8_t c)
{
    PORTB &= ~(1 << DISP_DC);   /* command mode */
    PORTB &= ~(1 << DISP_CS);
    spi_transfer(c);
    PORTB |=  (1 << DISP_CS);
}

static void oled_data(uint8_t d)
{
    PORTB |=  (1 << DISP_DC);   /* data mode */
    PORTB &= ~(1 << DISP_CS);
    spi_transfer(d);
    PORTB |=  (1 << DISP_CS);
}

void oled_goto(uint8_t page, uint8_t col)
{
    oled_cmd(0xB0 | (page & 0x07));   /* page 0-7           */
    oled_cmd(0x00 | (col & 0x0F));    /* column low nibble  */
    oled_cmd(0x10 | (col >> 4));      /* column high nibble */
}

void oled_clear(void)
{
    for (uint8_t page = 0; page < 8; page++) {
        oled_goto(page, 0);
        for (uint8_t col = 0; col < 128; col++)
            oled_data(0x00);
    }
}

void oled_init(void)
{
    _delay_ms(100);   /* let the display power up before talking to it */
    spi_init();

    oled_cmd(0xAE);   /* display off while configuring          */
    oled_cmd(0xA1);   /* flip horizontally                      */
    oled_cmd(0xC8);   /* flip vertically                        */
    oled_cmd(0x20);   /* memory addressing mode...              */
    oled_cmd(0x02);   /* ...page mode                           */
    oled_cmd(0xA4);   /* show RAM content (undoes 0xA5)         */
    oled_cmd(0xA6);   /* normal, not inverted (undoes 0xA7)     */
    oled_clear();
    oled_cmd(0xAF);   /* display on                             */
}

static const uint8_t *glyph(char c)
{
    if (c >= 'a' && c <= 'z')
        c -= 'a' - 'A';              /* lowercase drawn as uppercase */
    if (c >= 'A' && c <= 'Z')
        return letters[c - 'A'];
    if (c >= '0' && c <= '9')
        return digits[c - '0'];
    return blank;
}

void oled_putchar(char c)
{
    const uint8_t *g = glyph(c);
    for (uint8_t i = 0; i < 5; i++)
        oled_data(pgm_read_byte(&g[i]));
    oled_data(0x00);                  /* 1 blank column between characters */
}

void oled_print(const char *s)
{
    while (*s)
        oled_putchar(*s++);
}