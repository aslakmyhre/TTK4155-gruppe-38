#ifndef CAN_H
#define CAN_H

#include <stdbool.h>
#include <stdint.h>

/* Controller independent: nothing here names the MCP2515. Another controller
   (e.g. on node 2) implements these functions in its own can.c. */

#define CAN_MAX_STANDARD_ID 0x7FF
#define CAN_MAX_DATA_LENGTH 8

/* Standard (11-bit ID) data frame */
struct can_message {
    uint16_t id;
    uint8_t length;
    uint8_t data[CAN_MAX_DATA_LENGTH];
};

/* Normal mode at 125 kbit/s, bit timing matched to node 2. False if the
   controller does not respond, the bit timing does not read back, or it does
   not change mode. */
bool can_init(void);
/* False if id or length is out of range, or the controller is still busy
   sending the previous message. */
bool can_send(const struct can_message *message);
/* False if no message has arrived. Extended and remote frames are not
   supported and are dropped. */
bool can_receive(struct can_message *message);

#endif
