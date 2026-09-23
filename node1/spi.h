#ifndef SPI_H
#define SPI_H

#include <avr/io.h>
#include <stdint.h>

/* PORTB pin assignments. Hardware SS (PB4) must remain an output. */
#define SPI_IO_CS PB2
#define SPI_DISPLAY_CS PB1
#define SPI_DISPLAY_DC PB0
#define SPI_DISPLAY_RESET PB3

/* Every slave on the bus. Adding one (e.g. the CAN controller) is one entry
 * here and one CS pin in spi.c. */
enum spi_slave {
    SPI_SLAVE_IO,
    SPI_SLAVE_DISPLAY,
    SPI_SLAVE_COUNT
};

void spi_init(void);
uint8_t spi_transfer(uint8_t value);
void spi_write(const uint8_t *data, uint8_t count);
/* Deselects every other slave first, so two never drive MISO at once. */
void spi_select(enum spi_slave slave);
void spi_deselect_all(void);

#endif
