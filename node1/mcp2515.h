#ifndef MCP2515_H
#define MCP2515_H

#include <avr/io.h>
#include <stdbool.h>
#include <stdint.h>

/* INT is wired to INT0; the vector is fixed to this pin */
#define MCP2515_INT_PIN PD2

/* Register addresses, datasheet section 11 */
#define MCP2515_CANSTAT 0x0E
#define MCP2515_CANCTRL 0x0F
#define MCP2515_CANINTE 0x2B
#define MCP2515_CANINTF 0x2C

/* Same bit positions in CANINTE (enable) and CANINTF (flag) */
#define MCP2515_INT_RX0 0x01
#define MCP2515_INT_RX1 0x02

#define MCP2515_MAX_DATA_LENGTH 8

/* OPMOD in CANSTAT and REQOP in CANCTRL share bits 7-5 */
#define MCP2515_MODE_MASK     0xE0
#define MCP2515_MODE_NORMAL   0x00
#define MCP2515_MODE_LOOPBACK 0x40
#define MCP2515_MODE_CONFIG   0x80

/* Transmit buffers for mcp2515_request_to_send(), may be OR'ed */
#define MCP2515_TXB0 0x01
#define MCP2515_TXB1 0x02
#define MCP2515_TXB2 0x04

/* Bits of the byte returned by mcp2515_read_status(), figure 12-8 */
#define MCP2515_STATUS_RX0IF    0x01
#define MCP2515_STATUS_RX1IF    0x02
#define MCP2515_STATUS_TX0REQ   0x04
#define MCP2515_STATUS_TX0IF    0x08
#define MCP2515_STATUS_TX1REQ   0x10
#define MCP2515_STATUS_TX1IF    0x20
#define MCP2515_STATUS_TX2REQ   0x40
#define MCP2515_STATUS_TX2IF    0x80

/* SPI instruction set, datasheet chapter 12. Call spi_init() first. */
void mcp2515_reset(void);
uint8_t mcp2515_read(uint8_t address);
void mcp2515_read_array(uint8_t address, uint8_t *data, uint8_t count);
void mcp2515_write(uint8_t address, uint8_t value);
void mcp2515_write_array(uint8_t address, const uint8_t *data, uint8_t count);
/* False, and nothing sent, unless buffers is a non-empty MCP2515_TXBn mask */
bool mcp2515_request_to_send(uint8_t buffers);
uint8_t mcp2515_read_status(void);
/* Only for registers shaded in the register map; others take a full write */
void mcp2515_bit_modify(uint8_t address, uint8_t mask, uint8_t data);

/* Resets the controller, enables INT0 on the falling edge of INT and sets up
   reception: filters off, RXB0 overflows into RXB1, INT fires on a received
   frame. Global interrupts (sei) are left to the caller. False if the
   controller is not in configuration mode after reset, which usually means
   wiring, clock or CS is wrong. Stays in configuration mode. */
bool mcp2515_init(void);
/* True while an enabled CANINTF flag is set. Clear the flags with bit modify,
   then call again until false. */
bool mcp2515_interrupt_pending(void);
/* mode is one of MCP2515_MODE_*. False if the controller did not switch. */
bool mcp2515_set_mode(uint8_t mode);

/* Standard data frames through TXB0. The caller keeps id within 11 bits and
   length within MCP2515_MAX_DATA_LENGTH. False while TXB0 is still sending. */
bool mcp2515_transmit(uint16_t id, const uint8_t *data, uint8_t length);
/* data must hold MCP2515_MAX_DATA_LENGTH bytes. False if no frame is waiting.
   Extended and remote frames are not supported: they are dropped, and false
   is returned for them too. */
bool mcp2515_receive(uint16_t *id, uint8_t *data, uint8_t *length);

#endif
