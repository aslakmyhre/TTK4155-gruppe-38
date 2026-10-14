#ifndef MCP2515_H
#define MCP2515_H

#include <avr/io.h>
#include <stdbool.h>
#include <stdint.h>

#define MCP2515_INT_PIN PD2

//datasheet scetion 11
#define MCP2515_CANSTAT 0x0E //status
#define MCP2515_CANCTRL 0x0F //control
#define MCP2515_CANINTE 0x2B //interrupt enable
#define MCP2515_CANINTF 0x2C //interrupt flags

//interrupt bits
#define MCP2515_INT_RX0 0x01 
#define MCP2515_INT_RX1 0x02

#define MCP2515_MAX_DATA_LENGTH 8

//operating modes, register 10-1 and 10-2 in datasheet
#define MCP2515_MODE_MASK     0xE0 //1110 0000
#define MCP2515_MODE_NORMAL   0x00 //000x xxxx
#define MCP2515_MODE_LOOPBACK 0x40 //010x xxxx
#define MCP2515_MODE_CONFIG   0x80 //100x xxxx

//transmit buffers
#define MCP2515_TXB0 0x01
#define MCP2515_TXB1 0x02
#define MCP2515_TXB2 0x04

//read status bits
#define MCP2515_STATUS_RX0IF    0x01
#define MCP2515_STATUS_RX1IF    0x02
#define MCP2515_STATUS_TX0REQ   0x04
#define MCP2515_STATUS_TX0IF    0x08
#define MCP2515_STATUS_TX1REQ   0x10
#define MCP2515_STATUS_TX1IF    0x20
#define MCP2515_STATUS_TX2REQ   0x40
#define MCP2515_STATUS_TX2IF    0x80

// CNF1 Register Values
#define SJW1            0x00
#define SJW2            0x40
#define SJW3            0x80
#define SJW4            0xC0


// CNF2 Register Values
#define BTLMODE			0x80
#define SAMPLE_1X       0x00
#define SAMPLE_3X       0x40


// CNF3 Register Values
#define SOF_ENABLE		0x80
#define SOF_DISABLE		0x00
#define WAKFIL_ENABLE	0x40
#define WAKFIL_DISABLE	0x00


void mcp2515_reset(void);
uint8_t mcp2515_read(uint8_t address);
void mcp2515_read_array(uint8_t address, uint8_t *data, uint8_t count);
void mcp2515_write(uint8_t address, uint8_t value);
void mcp2515_write_array(uint8_t address, const uint8_t *data, uint8_t count);
bool mcp2515_request_to_send(uint8_t buffers);
uint8_t mcp2515_read_status(void);
void mcp2515_bit_modify(uint8_t address, uint8_t mask, uint8_t data);

//reset, setup, check alive
bool mcp2515_init(void);

//check interrupt
bool mcp2515_interrupt_pending(void);

bool mcp2515_set_mode(uint8_t mode);
bool mcp2515_transmit(uint16_t id, const uint8_t *data, uint8_t length);
bool mcp2515_receive(uint16_t *id, uint8_t *data, uint8_t *length);

//prints mode, error counters and flags over UART
void mcp2515_print_registers(void);

#endif
