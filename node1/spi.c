#include "spi.h"

//CS pin for each slave
static const uint8_t cs_pins[SPI_SLAVE_COUNT] = {
    [SPI_SLAVE_IO] = SPI_IO_CS,
    [SPI_SLAVE_DISPLAY] = SPI_DISPLAY_CS,
    [SPI_SLAVE_CAN] = SPI_CAN_CS,
};

//PORTB |= csmask() sets PB1,2,3 high
static uint8_t cs_mask(void)
{
    uint8_t mask = 0;
    for (uint8_t slave = 0; slave < SPI_SLAVE_COUNT; ++slave) {
        mask |= _BV(cs_pins[slave]);
    }
    return mask;
}

//set all CS pins high, no slave selected
void spi_deselect_all(void)
{
    PORTB |= cs_mask();
}

//select one slave
void spi_select(enum spi_slave slave)
{
    spi_deselect_all();
    //Pull CS low for selected slave
    PORTB &= (uint8_t)~_BV(cs_pins[slave]);
}

//send multiple bytes, ignore replies
void spi_write(const uint8_t *data, uint8_t count)
{
    for (uint8_t i = 0; i < count; ++i) {
        (void)spi_transfer(data[i]);
    }
}

void spi_init(void)
{
    spi_deselect_all();

    //set SS as input to keep MSTR bit
    PORTB |= _BV(PB4);
    DDRB |= _BV(PB4);

    //set CS pins, SCK, MISO as output
    PORTB &= (uint8_t)~(_BV(PB5) | _BV(PB6) | _BV(PB7));
    DDRB |= cs_mask() | _BV(PB5) | _BV(PB7);

    //MOSI as input
    DDRB &= (uint8_t)~_BV(PB6);

    //Mode 0, MSB first, F_CPU/128 = 38.4 kHz at 4.9152 MHz.
    SPCR = _BV(SPE) | _BV(MSTR) | _BV(SPR1) | _BV(SPR0);
    SPSR = 0;
}

//send one byte over SPI, return byte recieved
uint8_t spi_transfer(uint8_t value)
{
    SPDR = value;
    //wait until 8 bits have been sent
    while (!(SPSR & _BV(SPIF))) {}
    return SPDR;
}
