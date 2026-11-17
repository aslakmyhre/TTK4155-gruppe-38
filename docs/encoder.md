# Encoder — Node 2 (TTK4155)

## Principle

- The motor uses an **incremental quadrature encoder** with two signals, **A and B**, shifted by 90 electrical degrees.
- Their phase order indicates **direction**; counting transitions gives **relative position**.
- Unlike an absolute encoder, the count alone does **not** reveal position after power-up. We establish a reference by calibration.

## Hardware decoding (`node2/encoder.c`)

Node 2 uses the **SAM3X8E**. Its **TC2, channel 0 (TC6)** performs quadrature decoding in hardware, so the CPU does not process every encoder edge.

| Code / register | Purpose |
| --- | --- |
| `PMC->PMC_PCER1` / `ID_TC6` | Enable the TC6 peripheral clock. |
| `PIOC->PIO_PDR`, `PIOC->PIO_ABSR` | Assign **PC25/PC26** to peripheral B (timer inputs). |
| `TC2->TC_CHANNEL[0].TC_CMR` | Configure the timer channel for external/encoder input. |
| `TC_BMR_QDEN` | Enable quadrature decoder. |
| `TC_BMR_POSEN` | Enable position counting. |
| `TC_BMR_EDGPHA` | Enable edge-based phase decoding. |
| `TC_CCR_CLKEN \| TC_CCR_SWTRG` | Enable/start the counter. |
| `TC_CV` | Current position count, updated by hardware. |

```c
int32_t encoder_read(void) {
    return (int32_t)TC2->TC_CHANNEL[0].TC_CV;
}

void encoder_reset(void) {
    TC2->TC_CHANNEL[0].TC_CCR = TC_CCR_SWTRG;
}
```

**Key point:** No encoder ISR, software edge counting, or DMA is required: the timer peripheral maintains the count automatically.

## How the project uses it (`node2/main.c`)

1. `encoder_init()` configures the hardware decoder.
2. **Calibration:** drive to one mechanical end stop, reset the encoder count, then drive to the opposite end stop. The final count becomes `pos_max` (track range).
3. `joystick_to_ref()` maps joystick X to a target count, leaving a **5% margin** at each end.
4. Every **10 ms (100 Hz)**, the program reads `encoder_read()` and passes the position and reference to `controller_update()`.
5. The **PI position controller** (`node2/controller.c`) computes a signed motor command; `motor_set_speed()` applies the direction/PWM command. The controller output is limited, with basic anti-windup logic.

```text
Joystick X -> target position -> PI controller -> motor PWM/direction
                                    ^                    |
                                    |                    v
                            TC_CV encoder count <- motor/encoder
```

## Oral-exam essentials

- **Why two channels?** One pulse train indicates movement, but A/B phase order also identifies direction.
- **What does the TC peripheral do?** Decodes quadrature and updates the position counter without CPU intervention on each edge.
- **Why calibrate?** Incremental counts have no known absolute origin at startup; end-stop calibration establishes zero and travel range.
- **What is controlled?** Motor **position**, not directly speed: a 100 Hz PI loop converts position error into a motor speed/PWM command.
- **Node distinction:** Encoder decoding happens on the **SAM3X8E (Node 2)**; the **ATmega162 is Node 1**.

*Based on `node2/encoder.c`, `node2/controller.c`, `node2/controller.h`, and `node2/main.c` in the supplied project ZIP.*