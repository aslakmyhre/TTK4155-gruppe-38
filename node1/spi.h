#ifndef SPI_H
#define SPI_H

#include <avr/io.h>
#include <stdint.h>

#define SPI_IO_CS PB2
#define SPI_DISPLAY_CS PB1
#define SPI_DISPLAY_DC PB0
#define SPI_CAN_CS PB3
// PB4: Hardware SS


enum spi_slave {
    SPI_SLAVE_IO,
    SPI_SLAVE_DISPLAY,
    SPI_SLAVE_CAN,
    SPI_SLAVE_COUNT
};

void spi_init(void);
uint8_t spi_transfer(uint8_t value); //send 1 byte, recieve 1 byte
void spi_write(const uint8_t *data, uint8_t count); //ignore replies
void spi_select(enum spi_slave slave);
void spi_deselect_all(void);

#endif
