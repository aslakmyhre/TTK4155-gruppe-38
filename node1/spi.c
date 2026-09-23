#include "spi.h"
#include <util/delay.h>

static const uint8_t cs_pins[SPI_SLAVE_COUNT] = {
    [SPI_SLAVE_IO] = SPI_IO_CS,
    [SPI_SLAVE_DISPLAY] = SPI_DISPLAY_CS,
};

static bool transfer_failed;

bool spi_failed(void)
{
    return transfer_failed;
}

static uint8_t cs_mask(void)
{
    uint8_t mask = 0;
    for (uint8_t slave = 0; slave < SPI_SLAVE_COUNT; ++slave) {
        mask |= _BV(cs_pins[slave]);
    }
    return mask;
}

void spi_deselect_all(void)
{
    PORTB |= cs_mask();
}

void spi_select(enum spi_slave slave)
{
    spi_deselect_all();
    if (slave >= SPI_SLAVE_COUNT) {
        transfer_failed = true;
        return;
    }
    PORTB &= (uint8_t)~_BV(cs_pins[slave]);
}

void spi_write(const uint8_t *data, uint8_t count)
{
    for (uint8_t i = 0; i < count; ++i) {
        (void)spi_transfer(data[i]);
    }
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
    DDRB |= cs_mask() | _BV(PB5) | _BV(PB7);
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
