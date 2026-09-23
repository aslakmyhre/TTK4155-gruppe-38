#include "spi.h"
#include <util/delay.h>

static bool transfer_failed;

bool spi_failed(void)
{
    return transfer_failed;
}

void spi_deselect_all(void)
{
    PORTB |= _BV(SPI_IO_CS) | _BV(SPI_DISPLAY_CS);
}

void spi_init(void)
{
    transfer_failed = false;
    spi_deselect_all();
    /* PB4 is the fixed hardware SS pin, even when IO_CS uses PB2.
     * Leaving it as an input can clear MSTR when it goes low. */
    PORTB |= _BV(PB4);
    DDRB |= _BV(PB4);
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
    if (transfer_failed) {
        return 0;
    }
    if ((SPCR & (_BV(SPE) | _BV(MSTR))) != (_BV(SPE) | _BV(MSTR))) {
        transfer_failed = true;
        return 0;
    }
    SPDR = value;
    /* A byte takes about 208 us at our clock; allow at least 10 ms.
     * This checks the local SPI peripheral, not a slave acknowledgement. */
    for (uint16_t remaining = 1000; remaining != 0; --remaining) {
        if (!(SPCR & _BV(MSTR))) {
            break;
        }
        if (SPSR & _BV(SPIF)) {
            return SPDR;
        }
        _delay_us(10);
    }
    transfer_failed = true;
    return 0;
}
