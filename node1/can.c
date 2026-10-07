#include "can.h"
#include "mcp2515.h"

/* Implementation of can.h on the MCP2515 */

_Static_assert(CAN_MAX_DATA_LENGTH == MCP2515_MAX_DATA_LENGTH,
               "message data must fit the controller's frame");

#define CNF1 0x2A
#define CNF2 0x29
#define CNF3 0x28

/* 125 kbit/s with the 16 MHz crystal: TQ = 2 * (BRP + 1) / 16 MHz = 0.5 us,
   sync 1 + prop 7 + phase1 6 + phase2 2 = 16 TQ, sampled at 87.5 %.
   CAN_BR on node 2 must give the same segments. */
#define CNF1_VALUE 0x03 /* SJW 1 TQ, BRP 3 */
#define CNF2_VALUE 0xAE /* BTLMODE (phase2 from CNF3), 1 sample, PHSEG1 5, PRSEG 6 */
#define CNF3_VALUE 0x01 /* PHSEG2 1 */

bool can_init(void)
{
    /* Leaves the controller in configuration mode, the only mode where CNF1-3 can be written */
    if (!mcp2515_init()) {
        return false;
    }

    mcp2515_write(CNF1, CNF1_VALUE);
    mcp2515_write(CNF2, CNF2_VALUE);
    mcp2515_write(CNF3, CNF3_VALUE);
    /* The lab manual warns that write order matters, so check all three afterwards */
    if (mcp2515_read(CNF1) != CNF1_VALUE || mcp2515_read(CNF2) != CNF2_VALUE
        || mcp2515_read(CNF3) != CNF3_VALUE) {
        return false;
    }

    return mcp2515_set_mode(MCP2515_MODE_NORMAL);
}

bool can_send(const struct can_message *message)
{
    if (message->id > CAN_MAX_STANDARD_ID || message->length > CAN_MAX_DATA_LENGTH) {
        return false;
    }
    return mcp2515_transmit(message->id, message->data, message->length);
}

bool can_receive(struct can_message *message)
{
    return mcp2515_receive(&message->id, message->data, &message->length);
}
