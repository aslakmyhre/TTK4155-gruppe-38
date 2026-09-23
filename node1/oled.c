#include <util/delay.h>
#include "oled.h"
#include "spi.h"

static void begin(uint8_t data)
{
    spi_deselect_all();
    if (data) {
        PORTB |= _BV(SPI_DISPLAY_DC);
    } else {
        PORTB &= (uint8_t)~_BV(SPI_DISPLAY_DC);
    }
    PORTB &= (uint8_t)~_BV(SPI_DISPLAY_CS);
}

void oled_init(void)
{
    spi_deselect_all();
    PORTB |= _BV(SPI_DISPLAY_RESET);
    PORTB &= (uint8_t)~_BV(SPI_DISPLAY_DC);
    DDRB |= _BV(SPI_DISPLAY_RESET) | _BV(SPI_DISPLAY_DC);
    /* DISP_RES must connect ONLY to PB2, not also be tied to 5V by JP1.
     * The module supplies its own display power circuitry. */
    PORTB &= (uint8_t)~_BV(SPI_DISPLAY_RESET);
    _delay_ms(1);
    PORTB |= _BV(SPI_DISPLAY_RESET);
    _delay_ms(100);

    begin(0);
    (void)spi_transfer(0xAE); /* Display off while initializing RAM. */
    (void)spi_transfer(0x20); /* Page addressing mode. */
    (void)spi_transfer(0x02);
    (void)spi_transfer(0xA1); /* Segment remap: board orientation. */
    (void)spi_transfer(0xC8); /* Reverse COM scan. */
    (void)spi_transfer(0xA4); /* Display RAM contents. */
    (void)spi_transfer(0xA6); /* Normal, not inverted. */
    spi_deselect_all();

    oled_test_pattern(0);
    begin(0);
    (void)spi_transfer(0xAF); /* Display on. */
    spi_deselect_all();
}

void oled_test_pattern(uint8_t pattern)
{
    /* Stream pixels without allocating a 1024-byte framebuffer. */
    for (uint8_t page = 0; page < 8; ++page) {
        begin(0);
        (void)spi_transfer(0xB0 | page);
        (void)spi_transfer(0x00); /* Column low nibble. */
        (void)spi_transfer(0x10); /* Column high nibble. */
        spi_deselect_all();

        begin(1);
        for (uint8_t x = 0; x < 128; ++x) {
            uint8_t pixels = 0;
            if (pattern < 2) {
                pixels = ((x / 8 + page + pattern) & 1) ? 0xFF : 0x00;
            } else {
                for (uint8_t bit = 0; bit < 8; ++bit) {
                    uint8_t y = page * 8 + bit;
                    if (x == 0 || x == 127 || y == 0 || y == 63
                        || x / 2 == y || x / 2 == 63 - y) {
                        pixels |= (uint8_t)_BV(bit);
                    }
                }
            }
            (void)spi_transfer(pixels);
        }
        spi_deselect_all();
    }
}
