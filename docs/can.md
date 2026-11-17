# CAN — Quick Review (TTK4155)

## 1. Where CAN is used

- **Node 1 (ATmega162):** talks to an external **MCP2515 CAN controller over SPI**.
- **Node 2 (SAM3X8E):** uses its built-in **CAN0 peripheral**.
- Both connect to the shared **CAN_H/CAN_L bus through CAN transceivers**.
- Bus speed is configured to **125 kbit/s** on both nodes.

```text
Node 1: ATmega162 --SPI--> MCP2515 --transceiver--+
                                                   +-- CAN bus
Node 2: SAM3X8E/CAN0 -----------transceiver--------+
```

## 2. CAN message basics

A classic standard CAN data frame has an **11-bit identifier (ID)** and **0–8 data bytes**. CAN hardware handles framing, arbitration, CRC, and ACK.

Node 1 represents an application message as:

```c
struct can_message {
    uint16_t id;
    uint8_t length;
    uint8_t data[8];
};
```

- The **ID identifies the message type**, not a destination address. Nodes decide which messages to accept/process.
- During arbitration, a **lower numerical CAN ID has higher priority**.

This is an important core concept in how CAN is designed. Since we have

$$
\texttt{0x010 < 0x020}
$$

Then the joystick-messages will have priority over the score-messages, which is a reasonable design choice.

### Sending at Same Time

If two or more nodes start sending information at the same time, then as explained above, the one with the recessive bit can get preempted by the one with dominant bit. A dominant `0` will always win over a recessive `1`.

For instance, they start sending simultaneuosly

```
             ID bits
             ↓
Node 1:  00000010000
Node 2:  00000100000
               ↑
           first discrepancy
```

Here **node 1** sends `0`, **node 2** sends `1`, bus becomes `0`. **Node 2** now discover that it sent `1`, but read `0`. Then it knows that another more dominant message is being transmitted and preemptes its sending. **Node 1** continue sending without any restart. Thats why CAN-arbitration is called non-destructive arbitration. **Node 2** resumes its sending once the line is available again. 

**Node 2** will know when **Node 1** has finished sending its message usually by observing

- End of Frame (EOF), i.e. 7 recessive bits
- Intermission (3 recessive bits)

Following this the bus could be available, and another node could try to begin a new frame. The CAN-controller handles this logic automatically in hardware. 

**Messages in our project:**

| ID | Direction | Payload |
|---|---|---|
| `0x010` | Node 1 → Node 2 | 3 bytes: joystick X + 100, Y + 100, fire button |
| `0x020` | Node 2 → Node 1 | 2 bytes: score (`uint16_t`, low byte first) |

The actual CAN-message sent is

```c
struct can_message message = {
    .id = 0x010,
    .length = 3,
    .data = {
        (uint8_t)pos.x + 100,
        (uint8_t)pos.y + 100,
        fire ? 1 : 0
    },
};
```

Since joystick position originally is signed we shift its range from [-100,100] up to [0-200], so that it is fits neatly into an `uint8_t` variable. The actual sending of the message happens in the `can_send()`-function.

```c
bool can_send(const struct can_message *message)
{
    if (message->id > CAN_MAX_STANDARD_ID ||
        message->length > CAN_MAX_DATA_LENGTH) {
        return false;
    }

    return mcp2515_transmit(
        message->id,
        message->data,
        message->length
    );
}
```

We first control that the ID is < `0x7FF` (2047) and that the payload is < 64 bit ($N\leq8$). This is done since we originally define ID as `uint16_t` so higher ID's are possible to set in the code.

### SAM3X8E

Here we use can by directly talking to the memory mapped hardware registers. E.g.

```c
TC2->TC_BMR = value;
```


## 3. Sending and receiving

**Node 1:** `can_send()` validates ID/length, then calls `mcp2515_transmit()`. The ATmega162 sends commands/data **over SPI** to MCP2515, which handles the actual CAN transmission. `can_receive()` fetches a received message through the MCP2515 driver. Both functions can return `false` (e.g., transmit buffer busy / no message available).

**Node 2:** `can_try_tx()` queues a frame in a CAN0 transmit mailbox when ready; `can_rx()` reads a receive mailbox if a frame has arrived. The receive mailbox is configured to accept standard-ID frames. The application checks IDs and lengths before interpreting messages.

## 4. CAN errors vs application safety

**CAN hardware checks communication:**

- **CRC:** detects corrupted frames.
- **ACK:** another receiving node acknowledges a successfully received frame; missing ACK can trigger an error/retry.
- **TEC / REC:** transmit/receive error counters.
- **Error Active → Error Passive → Bus Off:** fault-confinement states; Bus Off means the controller stops participating in normal bus traffic.

**Debug registers used in our code:**

- **Node 1 (MCP2515):** `TEC`, `REC`, `EFLG` (plus `CANSTAT`, `TXB0CTRL`).
- **Node 2 (CAN0):** `CAN_SR` and `CAN_ECR`.

These are **hardware status/error indicators**, not custom error codes inside the joystick/score messages.

**Application-level safety:** If Node 2 receives no valid joystick message for **500 ms**, it centers the servo, stops the motor, and resets the controller. Hardware retries alone cannot replace this timeout.


## 5. Physical Implementation

CAN uses differential signal transfer. Every connected node have to be configured to the same transfer rate before successfull communication can occur.

### Differential Signal Transfer

Core concept relies on differentiating the total voltage difference between the two signals. For instance,


| State | CAN_H | CAN_L | Difference |
|---|---:|---:|---:|
| Recessive (`1`) | ca. 2,5 V | ca. 2,5 V | ca. 0 V |
| Dominant (`0`) | ca. 3,5 V | ca. 1,5 V | ca. 2 V |

Illustrated it could be represented as

```
           Recessive     Dominant      Recessive
              (1)           (0)           (1)

CAN_H  3.5V                ┌──────┐
                           │      │
       2.5V────────────────┘      └──────────────

CAN_L  2.5V────────────────┐      ┌──────────────
                           │      │
       1.5V                └──────┘

Difference:
        2V                 ┌──────┐
                           │      │
        0V ────────────────┘      └──────────────
```

Hence, 

- Small $\Delta V \to$ recessive `1`
- Large positive $\Delta V \to$ dominant `0`

This way of transfering information is useful, as it is more resilient to noise and in particular common-mode noise rejection, as it effects both lines similarly.

## 6. Key oral-exam points

- **SPI vs CAN:** SPI uses a master clock and chip-select lines; CAN is a shared, message-based bus using IDs and arbitration.
- **Why MCP2515?** ATmega162 accesses an external CAN controller via SPI; SAM3X8E has CAN hardware built in.
- **How does the recipient know what to use?** Inspect/filter the message **ID**, then decode the payload.
- **What if two nodes transmit at once?** CAN arbitrates non-destructively; the lower numerical ID wins.
- **What if communication fails?** CAN hardware detects errors and manages fault states; **our application** also uses a 500 ms joystick timeout for motor safety.

**Relevant files:** `node1/can.c`, `node1/can.h`, `node1/mcp2515.c`, `node1/main.c`, `node2/can.c`, `node2/can.h`, `node2/main.c`.

### MCP2515

We use an interrupt to detect when the MCP2515 requires attention. The ISR only sets a flag, allowing the CPU to quickly return to its previous task. The actual SPI communication is handled later in the main loop. This keeps interrupt execution short and avoids unnecessary SPI polling. 

This is particularly important as we run the SPI-line at quite slow frequency (38.4kHz).


