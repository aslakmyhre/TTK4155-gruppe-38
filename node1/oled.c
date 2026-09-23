#include <avr/io.h>
#include "oled.h"
#include "xmem.h"

static volatile uint8_t *const fb = (uint8_t *) SRAM_BASE;

/* 5x7 glyphs, one byte per column, bit 0 = top pixel */
static const uint8_t digit_glyphs[10][5] = {
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
static const uint8_t minus_glyph[5] = {0x08,0x08,0x08,0x08,0x08};
static const uint8_t x_glyph[5]     = {0x63,0x14,0x08,0x14,0x63};
static const uint8_t y_glyph[5]     = {0x07,0x08,0x70,0x08,0x07};
static const uint8_t blank_glyph[5] = {0};


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
    oled_cmd(0xB0 | (page & 0x07));   /* page 0-7           */
    oled_cmd(0x00 | (col & 0x0F));    /* column low nibble  */
    oled_cmd(0x10 | (col >> 4));      /* column high nibble */
}

void oled_fill(uint8_t pattern)
{
    for (uint8_t p = 0; p < OLED_PAGES; p++) {
        oled_goto(p, 0);
        for (uint8_t c = 0; c < OLED_WIDTH; c++)
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


/* ---------------- Framebuffer drawing ---------------- */

static const uint8_t *glyph(char c)
{
    if (c >= '0' && c <= '9')
        return digit_glyphs[c - '0'];
    switch (c) {
        case '-': return minus_glyph;
        case 'X': return x_glyph;
        case 'Y': return y_glyph;
    }
    return blank_glyph;
}

void oled_fb_clear(void)
{
    for (uint16_t i = 0; i < OLED_WIDTH * OLED_PAGES; i++)
        fb[i] = 0;
}

void oled_pixel(uint8_t x, uint8_t y)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT)
        return;
    fb[(y / 8) * OLED_WIDTH + x] |= 1 << (y % 8);
}

void oled_rect(uint8_t x0, uint8_t y0, uint8_t w, uint8_t h)
{
    for (uint8_t i = 0; i < w; i++) {
        oled_pixel(x0 + i, y0);
        oled_pixel(x0 + i, y0 + h - 1);
    }
    for (uint8_t j = 0; j < h; j++) {
        oled_pixel(x0, y0 + j);
        oled_pixel(x0 + w - 1, y0 + j);
    }
}

void oled_fill_rect(uint8_t x0, uint8_t y0, uint8_t w, uint8_t h)
{
    for (uint8_t j = 0; j < h; j++)
        for (uint8_t i = 0; i < w; i++)
            oled_pixel(x0 + i, y0 + j);
}

void oled_text(uint8_t page, uint8_t x, const char *s)
{
    for (; *s; s++) {
        const uint8_t *g = glyph(*s);
        for (uint8_t col = 0; col < 5 && x < OLED_WIDTH; col++, x++)
            fb[page * OLED_WIDTH + x] = g[col];
        x++;    /* 1 blank column between characters */
    }
}

void oled_flush(void)
{
    for (uint8_t page = 0; page < OLED_PAGES; page++) {
        oled_goto(page, 0);
        /* Keep CS low for the whole page instead of toggling per byte */
        PORTB |=  (1 << DISP_DC);
        PORTB &= ~(1 << DISP_CS);
        for (uint8_t col = 0; col < OLED_WIDTH; col++)
            spi_transfer(fb[page * OLED_WIDTH + col]);
        PORTB |=  (1 << DISP_CS);
    }
}