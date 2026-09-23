#include "spi.h"

void spi_deselect_all(void)
{
    PORTB |= _BV(SPI_IO_CS) | _BV(SPI_DISPLAY_CS);
}

void spi_init(void)
{
    spi_deselect_all();
    PORTB &= (uint8_t)~(_BV(PB5) | _BV(PB6) | _BV(PB7));
    DDRB |= _BV(SPI_IO_CS) | _BV(SPI_DISPLAY_CS)
          | _BV(PB5) | _BV(PB7);
    DDRB &= (uint8_t)~_BV(PB6);

    /* Mode 0, MSB first, F_CPU/128 = 38.4 kHz at 4.9152 MHz.
     * The IO-board PDF does not specify mode, bit order or maximum
     * clock rate: these are starting settings to verify on hardware. */
    SPCR = _BV(SPE) | _BV(MSTR) | _BV(SPR1) | _BV(SPR0);
    SPSR = 0;
}

uint8_t spi_transfer(uint8_t value)
{
    SPDR = value;
    while (!(SPSR & _BV(SPIF))) {}
    return SPDR;
}
