# TTK4155 — System Configuration & Design Choices

Quick reference for the oral evaluation. Values below come from the group's source code unless marked **lab manual**. The reasoning is an engineering interpretation, not necessarily a documented original design decision.

## Communication

| Setting | Configured value | Where | Why it matters |
|---|---:|---|---|
| Node 1 CPU clock | 4.9152 MHz | Node 1 build configuration / SPI comment | Reference clock for SPI and other timing |
| SPI clock | **38.4 kHz** (`F_CPU / 128`) | `node1/spi.c`, `spi_init()` | Shared by IO board, display, MCP2515; intentionally or accidentally conservative |
| SPI mode | Mode 0, MSB first | `node1/spi.c` | Master and slaves must agree on clock polarity/phase |
| MCP2515 oscillator | **16 MHz** | `node1/can.c` comments | CAN bit-time generation, **not** the AVR's SPI clock |
| CAN bitrate | **125 kbit/s** | `node1/can.c` `CNF1–3`; `node2/main.c` `can_init()` | Both nodes must use compatible bit timing |
| CAN bit duration | **8 µs** | 16 time quanta × 0.5 µs | Gives 125 kbit/s; sample point 87.5% |
| CAN joystick ID | `0x010`, 3 bytes | Both `main.c` files | X, Y, fire; higher arbitration priority than score |
| CAN score ID | `0x020`, 2 bytes | Both `main.c` files | 16-bit score, little-endian |

**Why is SPI only 38.4 kHz?** The lab manual does **not** prescribe this specific clock. Exercises 4–5 instead ask students to consider shared-bus bandwidth, contention, and to calculate achievable update rates. The implementation uses the largest standard SPI prescaler (`SPR1=SPR0=1`, `SPI2X=0`). We could likely increase the clock subject to all slaves' electrical/timing limits and re-test. Do not claim the rate was chosen for a specific reason unless the team remembers/testing confirms it.

> We initially used a conservative SPI clock of 38.4 kHz. This was sufficient for basic communication, but in retrospect we could have increased it to reduce transfer times, particularly for OLED updates. We did not identify SPI throughput as a critical issue during development.

**Useful throughput calculation:** At 38.4 kHz, one 8-bit SPI transfer needs ~208 µs (not including software overhead). Three transfers to read one MCP2515 register take at least ~625 µs. A hypothetical contiguous 1024-byte OLED payload needs at least ~213 ms on the SPI bus, so screen updates may benefit more from a speed increase than small CAN messages. Actual display transactions include protocol overhead and may not send a whole framebuffer at once.

**Why CAN at 125 kbit/s?** Small control messages do not need high throughput; using a standard moderate bitrate is reasonable. More importantly, both CAN controllers must agree on bitrate and compatible sampling/timing. The source configures 16 TQ/bit, 0.5 µs each. The lab manual requires matching CAN bit timing but does **not** mandate 125 kbit/s.

## Periodic Tasks & Timeouts

| Task | Value | Where | Interpretation |
|---|---:|---|---|
| Game joystick send attempt | **every ~50 ms** | `node1/main.c` `GAME_PERIOD_MS` | Target 20 updates/s; blocking I/O adds overhead; send can fail if TX buffer is busy |
| CAN loopback test iteration | **50 ms delay** | `node1/main.c` `CAN_TEST_PERIOD_MS` | Diagnostic/test screen, not the game update rate guarantee |
| Score sending | **100 ms** | `node2/main.c` `SCORE_SEND_PERIOD_MS` | Periodic score update, even without a new goal |
| Joystick loss timeout | **500 ms** | `node2/main.c` `JOYSTICK_TIMEOUT_MS` | Centre servo, stop motor, reset PI controller |
| PI position controller | **10 ms** | `node2/controller.h` `CONTROLLER_PERIOD_MS` | Target 100 Hz controller loop |
| Node 1 input-screen delay | **200 ms** | `node1/main.c` `SCREEN_PERIOD_MS` | Approx. 5 screen iterations/s before I/O overhead |
| Node 2 CAN diagnostic print | **500 ms** | `node2/main.c` `CAN_DEBUG_PERIOD_MS` | Avoid printing constantly |
| Node 2 PI diagnostic print | **200 ms** | `node2/main.c` `PI_DEBUG_PERIOD_MS` | Comment says faster UART printing may starve 10 ms control loop |

**Common exam trap:** **50 ms** is the nominal joystick transmit interval; **500 ms** is the timeout if frames stop arriving. These are different design choices. The busy-wait `_delay_ms()` and other blocking operations mean the real interval may be longer than the defined delay.

## Actuators, Calibration & Safety

| Setting | Value | Where | Meaning |
|---|---:|---|---|
| Servo PWM frequency | **50 Hz** | `node2/pwm.c` `PERIOD_US=20000` | One PWM period is 20 ms |
| Servo pulse | **900–2100 µs** | `node2/pwm.c` | Min/max command pulse widths |
| Servo centre | **1500 µs** | `node2/pwm.c` | Neutral command |
| PWM timer clock | **656,250 Hz** | `node2/pwm.c` | 84 MHz master clock / 128 |
| Calibration motor command | **30** (driver units) | `node2/main.c` `CAL_SPEED` | Drives to each track end |
| Calibration stationary threshold | **300 ms** | `node2/main.c` `CAL_STALL_MS` | No detected encoder movement for this long means stall/end stop |
| Calibration timeout | **5000 ms** | `node2/main.c` `CAL_TIMEOUT_MS` | Prevents endless movement during calibration |
| Encoder movement threshold | **5 counts** | `node2/main.c` `CAL_MOVE_COUNTS` | Minimum movement to update stall detector |
| Minimum valid encoder span | **5000 counts** | `node2/main.c` `CAL_MIN_RANGE` | Rejects implausibly short calibration |

## How to Discuss Design Choices

- **What is configured?** Point to the `#define` or the hardware register setup.
- **What is required?** Distinguish an assignment constraint or a device datasheet limit from a team decision.
- **Why this value?** Explain the tradeoff (latency, bandwidth, noise margin, CPU load, safety), but do not invent a historical motivation.
- **What could improve?** Faster SPI may help OLED transfers; better measurement of loop timing would establish whether the update intervals are achieved; avoid blocking debug output in time-critical paths.

## Lab Manual Pointers

- **Exercise 4, §3.4.2–3.4.4 (printed pp. 36–37):** SPI operation, shared slaves, bandwidth considerations.
- **Exercise 5, §3.5.3–3.5.5 (printed pp. 39–40):** SPI bus contention and calculation of transfer frequency; layering SPI → MCP2515 → CAN messages.
- **Exercise 6, §3.6.4 (printed pp. 42–43):** CAN bit timing and matching timing parameters across nodes.

*Review this file against the actual running setup if the firmware has been changed since the archive was exported.*
