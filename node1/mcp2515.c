#include <util/delay.h>
#include "mcp2515.h"
#include "spi.h"

/* Datasheet table 12-1 */
#define INSTRUCTION_RESET       0xC0
#define INSTRUCTION_READ        0x03
#define INSTRUCTION_WRITE       0x02
#define INSTRUCTION_RTS         0x80
#define INSTRUCTION_READ_STATUS 0xA0
#define INSTRUCTION_BIT_MODIFY  0x05

#define TXB_ALL (MCP2515_TXB0 | MCP2515_TXB1 | MCP2515_TXB2)

/* The oscillator start-up timer holds the chip for 128 OSC1 cycles,
   8 us with the 16 MHz crystal (datasheet 8.1); wait with margin. */
#define RESET_WAIT_US 100

/* Every instruction starts with CS going low; the chip takes the first byte
   after that as the instruction, so each one needs its own select. */
static void begin(uint8_t instruction)
{
    spi_select(SPI_SLAVE_CAN);
    (void)spi_transfer(instruction);
}

void mcp2515_reset(void)
{
    begin(INSTRUCTION_RESET);
    spi_deselect_all();
    _delay_us(RESET_WAIT_US);
}

void mcp2515_read_array(uint8_t address, uint8_t *data, uint8_t count)
{
    begin(INSTRUCTION_READ);
    (void)spi_transfer(address);
    for (uint8_t i = 0; i < count; ++i) {
        data[i] = spi_transfer(0x00);
    }
    spi_deselect_all();
}

uint8_t mcp2515_read(uint8_t address)
{
    uint8_t value;
    mcp2515_read_array(address, &value, 1);
    return value;
}

void mcp2515_write_array(uint8_t address, const uint8_t *data, uint8_t count)
{
    begin(INSTRUCTION_WRITE);
    (void)spi_transfer(address);
    spi_write(data, count);
    spi_deselect_all();
}

void mcp2515_write(uint8_t address, uint8_t value)
{
    mcp2515_write_array(address, &value, 1);
}

bool mcp2515_request_to_send(uint8_t buffers)
{
    /* Stray high bits would turn this into a different instruction */
    if (buffers == 0 || (buffers & (uint8_t)~TXB_ALL)) {
        return false;
    }
    begin(INSTRUCTION_RTS | buffers);
    spi_deselect_all();
    return true;
}

uint8_t mcp2515_read_status(void)
{
    begin(INSTRUCTION_READ_STATUS);
    uint8_t status = spi_transfer(0x00);
    spi_deselect_all();
    return status;
}

void mcp2515_bit_modify(uint8_t address, uint8_t mask, uint8_t data)
{
    const uint8_t bytes[] = {address, mask, data};
    begin(INSTRUCTION_BIT_MODIFY);
    spi_write(bytes, sizeof bytes);
    spi_deselect_all();
}

bool mcp2515_init(void)
{
    mcp2515_reset();
    return (mcp2515_read(MCP2515_CANSTAT) & MCP2515_MODE_MASK) == MCP2515_MODE_CONFIG;
}
