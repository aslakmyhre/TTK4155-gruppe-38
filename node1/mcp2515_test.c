#include <avr/pgmspace.h>
#include <stdbool.h>
#include <stdio.h>
#include <util/delay.h>
#include "mcp2515.h"
#include "mcp2515_test.h"
#include "oled.h"

#define PATTERN       0xA5
#define MODIFY_MASK   0x0F
#define MODIFIED      0xA0   /* PATTERN with MODIFY_MASK bits cleared */
#define CNF1_RESET    0x00   /* with CNF2/3 at reset too, a valid bit timing */

/* Standard ID 0x123: SIDH holds ID bits 10-3, SIDL bits 7-5 hold ID bits 2-0 */
#define TEST_SIDH     0x24
#define TEST_SIDL     0x60
#define TEST_DLC      1
#define TEST_DATA     0x42

#define RX_TIMEOUT_MS 10     /* one frame in loopback takes well under 1 ms */

static void show(uint8_t line, const char *name, bool ok, uint8_t value)
{
    oled_pos(line, 0);
    fprintf_P(oled_output(), PSTR("%-7S %-4S %02X"),
              name, ok ? PSTR("OK") : PSTR("FAIL"), value);
}

static uint8_t wait_for_rx0(void)
{
    uint8_t status = mcp2515_read_status();
    for (uint8_t ms = 0; ms < RX_TIMEOUT_MS && !(status & MCP2515_STATUS_RX0IF); ms++) {
        _delay_ms(1);
        status = mcp2515_read_status();
    }
    return status;
}

void mcp2515_test(void)
{
    oled_clear();

    bool ok = mcp2515_init();
    show(0, PSTR("RESET"), ok, mcp2515_read(MCP2515_CANSTAT));

    mcp2515_write(MCP2515_CNF1, PATTERN);
    uint8_t value = mcp2515_read(MCP2515_CNF1);
    show(1, PSTR("WRITE"), value == PATTERN, value);

    mcp2515_bit_modify(MCP2515_CNF1, MODIFY_MASK, 0x00);
    value = mcp2515_read(MCP2515_CNF1);
    show(2, PSTR("BITMOD"), value == MODIFIED, value);
    mcp2515_write(MCP2515_CNF1, CNF1_RESET);

    mcp2515_bit_modify(MCP2515_CANCTRL, MCP2515_MODE_MASK, MCP2515_MODE_LOOPBACK);
    value = mcp2515_read(MCP2515_CANSTAT) & MCP2515_MODE_MASK;
    show(3, PSTR("LOOPBK"), value == MCP2515_MODE_LOOPBACK, value);

    const uint8_t frame[] = {TEST_SIDH, TEST_SIDL, 0x00, 0x00, TEST_DLC, TEST_DATA};
    mcp2515_write_array(MCP2515_TXB0SIDH, frame, sizeof frame);
    mcp2515_write(MCP2515_CANINTE, MCP2515_INT_RX0);
    ok = mcp2515_request_to_send(MCP2515_TXB0);
    uint8_t status = wait_for_rx0();
    show(4, PSTR("RTS"), ok && (status & MCP2515_STATUS_RX0IF), status);

    value = mcp2515_read(MCP2515_RXB0D0);
    show(5, PSTR("RX DATA"), value == TEST_DATA, value);

    show(6, PSTR("INT PIN"), mcp2515_interrupt_pending(), mcp2515_read(MCP2515_CANINTF));

    /* Clears CANINTE/CANINTF and leaves loopback, as after startup */
    mcp2515_reset();
}
