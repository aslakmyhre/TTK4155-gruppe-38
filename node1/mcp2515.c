#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/delay.h>
#include "mcp2515.h"
#include "spi.h"

//table 12-1
#define INSTRUCTION_RESET       0xC0 //1100 0000
#define INSTRUCTION_READ        0x03 //0000 0011
#define INSTRUCTION_WRITE       0x02 //0000 0010
#define INSTRUCTION_RTS         0x80 //1000 0nnn (nnn decides buffer, see datasheet)
#define INSTRUCTION_READ_STATUS 0xA0 //1010 0000
#define INSTRUCTION_BIT_MODIFY  0x05 //0000 0101
#define INSTRUCTION_READ_RX_BUFFER 0x90 //1001 0nm0 - reading recieve buffer
#define INSTRUCTION_LOAD_TX_BUFFER 0x40 //0100 0abc - writing transmit buffer
#define READ_RX_BUFFER_SHIFT       2

#define RXB0CTRL 0x60
#define RXB1CTRL 0x70
#define RXBCTRL_RXM_ANY 0x60 //0110 0000 - accept any
#define RXBCTRL_BUKT    0x04 //0000 0100 - RXB0 full

#define FRAME_HEADER_LENGTH 5
#define HEADER_SIDH 0
#define HEADER_SIDL 1
#define HEADER_DLC  4
#define SIDH_SHIFT  3
#define SIDL_SHIFT  5
#define SIDL_ID_MASK 0x07
#define SIDL_SRR    0x10
#define SIDL_IDE    0x08
#define DLC_MASK    0x0F

#define MODE_SWITCH_TIMEOUT_MS 10

#define TXB_ALL (MCP2515_TXB0 | MCP2515_TXB1 | MCP2515_TXB2)

//wait for crystal to stabilize
#define RESET_WAIT_US 100

static volatile bool interrupt_flag;

ISR(INT0_vect)
{
    interrupt_flag = true;
}

static void int0_init(void)
{
    DDRD &= (uint8_t)~_BV(MCP2515_INT_PIN); //PD2=input
    MCUCR = (uint8_t)((MCUCR & ~_BV(ISC00)) | _BV(ISC01)); //trigger on falling edge
    GIFR = _BV(INTF0); //clear old trigger
    GICR |= _BV(INT0); //enable INT0
}

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
        data[i] = spi_transfer(0x00); //send dummy, recieve register value
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
    // Stray high bits would turn this into a different instruction
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

static void receive_init(void)
{
    mcp2515_write(RXB0CTRL, RXBCTRL_RXM_ANY | RXBCTRL_BUKT); //0x64 - accept all + rollover
    mcp2515_write(RXB1CTRL, RXBCTRL_RXM_ANY); //0x60 accept all
    mcp2515_write(MCP2515_CANINTE, MCP2515_INT_RX0 | MCP2515_INT_RX1);
}

bool mcp2515_init(void)
{
    mcp2515_reset();
    int0_init();
    if ((mcp2515_read(MCP2515_CANSTAT) & MCP2515_MODE_MASK) != MCP2515_MODE_CONFIG) {
        return false;
    }
    receive_init();
    return true;
}

bool mcp2515_set_mode(uint8_t mode)
{
    mcp2515_bit_modify(MCP2515_CANCTRL, MCP2515_MODE_MASK, mode);
    for (uint8_t ms = 0; ms < MODE_SWITCH_TIMEOUT_MS; ms++) {
        if ((mcp2515_read(MCP2515_CANSTAT) & MCP2515_MODE_MASK) == mode) {
            return true;
        }
        _delay_ms(1);
    }
    return false;
}

bool mcp2515_transmit(uint16_t id, const uint8_t *data, uint8_t length)
{
    if (mcp2515_read_status() & MCP2515_STATUS_TX0REQ) {
        return false;
    }
    const uint8_t header[FRAME_HEADER_LENGTH] = {
        [HEADER_SIDH] = (uint8_t)(id >> SIDH_SHIFT),
        [HEADER_SIDL] = (uint8_t)((id & SIDL_ID_MASK) << SIDL_SHIFT),
        [HEADER_DLC]  = length,
    };
    begin(INSTRUCTION_LOAD_TX_BUFFER);
    spi_write(header, sizeof header);
    spi_write(data, length);
    spi_deselect_all();
    return mcp2515_request_to_send(MCP2515_TXB0);
}

/* Raising CS at the end of READ RX BUFFER clears the buffer's RXnIF */
static bool read_rx_buffer(uint8_t buffer, uint16_t *id, uint8_t *data, uint8_t *length)
{
    uint8_t header[FRAME_HEADER_LENGTH];
    begin(INSTRUCTION_READ_RX_BUFFER | (uint8_t)(buffer << READ_RX_BUFFER_SHIFT));
    for (uint8_t i = 0; i < FRAME_HEADER_LENGTH; ++i) {
        header[i] = spi_transfer(0x00);
    }
    if (header[HEADER_SIDL] & (SIDL_IDE | SIDL_SRR)) {
        spi_deselect_all();
        return false;
    }
    uint8_t dlc = header[HEADER_DLC] & DLC_MASK;
    /* CAN 2.0B: a DLC of 9-15 still carries 8 data bytes */
    *length = dlc > MCP2515_MAX_DATA_LENGTH ? MCP2515_MAX_DATA_LENGTH : dlc;
    for (uint8_t i = 0; i < *length; ++i) {
        data[i] = spi_transfer(0x00);
    }
    spi_deselect_all();
    *id = (uint16_t)((uint16_t)header[HEADER_SIDH] << SIDH_SHIFT
                     | header[HEADER_SIDL] >> SIDL_SHIFT);
    return true;
}

bool mcp2515_receive(uint16_t *id, uint8_t *data, uint8_t *length)
{
    /* The INT pin says whether anything arrived without an SPI transfer */
    if (!mcp2515_interrupt_pending()) {
        return false;
    }
    uint8_t status = mcp2515_read_status();
    if (status & MCP2515_STATUS_RX0IF) {
        return read_rx_buffer(0, id, data, length);
    }
    if (status & MCP2515_STATUS_RX1IF) {
        return read_rx_buffer(1, id, data, length);
    }
    return false;
}

bool mcp2515_interrupt_pending(void)
{
    bool edge_seen;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        edge_seen = interrupt_flag;
        interrupt_flag = false; //clear flag
    }
    bool pin_low = !(PIND & _BV(MCP2515_INT_PIN));
    return edge_seen || pin_low;
}
