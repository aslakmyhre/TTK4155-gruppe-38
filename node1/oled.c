#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "oled.h"
#include "spi.h"

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


static void oled_cmd(uint8_t c)
{
    spi_deselect_all();
    PORTB &= ~(1 << SPI_DISPLAY_DC);   /* command mode */
    PORTB &= ~(1 << SPI_DISPLAY_CS);
    (void)spi_transfer(c);
    PORTB |=  (1 << SPI_DISPLAY_CS);
}

static void oled_data(uint8_t d)
{
    spi_deselect_all();
    PORTB |=  (1 << SPI_DISPLAY_DC);   /* data mode */
    PORTB &= ~(1 << SPI_DISPLAY_CS);
    (void)spi_transfer(d);
    PORTB |=  (1 << SPI_DISPLAY_CS);
}

static void oled_reset(void)
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

/* Vertical format: each byte is 8 stacked pixels, bit 0 = top.
   This is what the display expects, so bytes are sent as they are. */
void oled_draw_image(uint8_t x, uint8_t page, uint8_t width, uint8_t height,
                     const uint8_t *img)
{
    for (uint8_t p = 0; p < height / 8; p++) {
        oled_goto(page + p, x);
        for (uint8_t col = 0; col < width; col++)
            oled_data(pgm_read_byte(&img[p * width + col]));
    }
}

/* Horizontal format: each byte is 8 pixels side by side, MSB = leftmost.
   Collects one bit from each of 8 rows to build one display byte. */
static uint8_t image_h_byte(const uint8_t *img, uint8_t width,
                            uint8_t p, uint8_t col)
{
    uint8_t bytes_per_row = width / 8;
    uint8_t out = 0;

    for (uint8_t bit = 0; bit < 8; bit++) {
        uint8_t row = p * 8 + bit;
        uint8_t b = pgm_read_byte(&img[row * bytes_per_row + col / 8]);
        if (b & (0x80 >> (col % 8)))
            out |= 1 << bit;
    }
    return out;                       /* use ~out to invert the colors */
}

void oled_draw_image_h(uint8_t x, uint8_t page, uint8_t width, uint8_t height,
                       const uint8_t *img)
{
    for (uint8_t p = 0; p < height / 8; p++) {
        oled_goto(page + p, x);
        for (uint8_t col = 0; col < width; col++)
            oled_data(image_h_byte(img, width, p, col));
    }
}

void oled_draw_image_h_page(uint8_t x, uint8_t page, uint8_t width,
                            const uint8_t *img, uint8_t p, uint8_t mask)
{
    oled_goto(page, x);
    for (uint8_t col = 0; col < width; col++)
        oled_data(image_h_byte(img, width, p, col) & mask);
}
