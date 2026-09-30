#ifndef MCP2515_H
#define MCP2515_H

#include <stdbool.h>
#include <stdint.h>

/* Register addresses, datasheet section 11 */
#define MCP2515_CANSTAT 0x0E
#define MCP2515_CANCTRL 0x0F

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

/* Resets the controller. False if it is not in configuration mode afterwards,
   which usually means wiring, clock or CS is wrong. */
bool mcp2515_init(void);

#endif
