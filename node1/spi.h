#ifndef SPI_H
#define SPI_H

#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>

/* PORTB pin assignments. Hardware SS (PB4) must remain an output. */
#define SPI_IO_CS PB2
#define SPI_DISPLAY_CS PB1
#define SPI_DISPLAY_DC PB0
#define SPI_DISPLAY_RESET PB3

void spi_init(void);
uint8_t spi_transfer(uint8_t value);
void spi_deselect_all(void);
/* Latched local transfer failure; reset by spi_init(). Discard received
 * data when true. No slave-presence detection is provided by SPI. */
bool spi_failed(void);

#endif
