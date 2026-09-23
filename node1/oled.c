#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "fonts.h"
#include "oled.h"
#include "spi.h"

/* font5 from fonts.h: 95 glyphs from ' ' to '~', 5 columns each */
#define FONT_WIDTH      5
#define FONT_FIRST_CHAR ' '
#define FONT_LAST_CHAR  '~'

static uint8_t current_line;


static void oled_cmd(uint8_t c)
{
    PORTB &= ~(1 << SPI_DISPLAY_DC);   /* command mode */
    spi_select(SPI_SLAVE_DISPLAY);
    (void)spi_transfer(c);
    spi_deselect_all();
}

static void oled_data(uint8_t d)
{
    PORTB |=  (1 << SPI_DISPLAY_DC);   /* data mode */
    spi_select(SPI_SLAVE_DISPLAY);
    (void)spi_transfer(d);
    spi_deselect_all();
}

void oled_reset(void)
{
    PORTB |= (1 << SPI_DISPLAY_RESET);
    PORTB &= ~(1 << SPI_DISPLAY_DC);
    DDRB  |= (1 << SPI_DISPLAY_RESET) | (1 << SPI_DISPLAY_DC);
    /* DISP_RES must connect ONLY to PB3, not also be tied to 5V by JP1 */
    PORTB &= ~(1 << SPI_DISPLAY_RESET);
    _delay_ms(1);
    PORTB |=  (1 << SPI_DISPLAY_RESET);
    _delay_ms(100);   /* let the display power up before talking to it */
}

void oled_goto_line(uint8_t line)
{
    current_line = line % OLED_LINES;
    oled_cmd(0xB0 | current_line);    /* page 0-7 */
}

void oled_goto_column(uint8_t column)
{
    oled_cmd(0x00 | (column & 0x0F)); /* column low nibble  */
    oled_cmd(0x10 | (column >> 4));   /* column high nibble */
}

void oled_pos(uint8_t line, uint8_t column)
{
    oled_goto_line(line);
    oled_goto_column(column);
}

void oled_home(void)
{
    oled_pos(0, 0);
}

void oled_clear_line(uint8_t line)
{
    oled_pos(line, 0);
    for (uint8_t col = 0; col < OLED_COLUMNS; col++)
        oled_data(0x00);
    oled_pos(line, 0);
}

void oled_clear(void)
{
    for (uint8_t line = 0; line < OLED_LINES; line++)
        oled_clear_line(line);
    oled_home();
}

void oled_contrast(uint8_t level)
{
    oled_cmd(0x81);
    oled_cmd(level);
}

void oled_init(void)
{
    oled_reset();

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

void oled_putchar(char c)
{
    if (c < FONT_FIRST_CHAR || c > FONT_LAST_CHAR)
        c = '?';
    const uint8_t *glyph = font5[c - FONT_FIRST_CHAR];
    for (uint8_t i = 0; i < FONT_WIDTH; i++)
        oled_data(pgm_read_byte(&glyph[i]));
    oled_data(0x00);                  /* 1 blank column between characters */
}

void oled_print(const char *s)
{
    while (*s)
        oled_putchar(*s++);
}

static int oled_stream_putchar(char c, FILE *stream)
{
    (void)stream;
    if (c == '\n')
        oled_pos(current_line + 1, 0);
    else
        oled_putchar(c);
    return 0;
}

static FILE oled_stream = FDEV_SETUP_STREAM(oled_stream_putchar, NULL, _FDEV_SETUP_WRITE);

FILE *oled_output(void)
{
    return &oled_stream;
}
