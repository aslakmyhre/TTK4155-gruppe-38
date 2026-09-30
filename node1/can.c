#include "can.h"
#include "mcp2515.h"

/* Implementation of can.h on the MCP2515 */

_Static_assert(CAN_MAX_DATA_LENGTH == MCP2515_MAX_DATA_LENGTH,
               "message data must fit the controller's frame");

bool can_init(void)
{
    /* TODO(exercise 3.6): set bit timing (CNF1-3) and use normal mode to
       talk to node 2; loopback needs neither. */
    return mcp2515_init() && mcp2515_set_mode(MCP2515_MODE_LOOPBACK);
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
