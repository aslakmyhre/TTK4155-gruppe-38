# Motor Control — Node 2 (TTK4155)

Quick reference based on `node2/motor.c`, `controller.c`, `encoder.c`, `servo.c`, `solenoid.c`, and `main.c`.

## System overview

```text
Joystick X (CAN) -> target encoder count -> PI position controller -> signed motor PWM/DIR
                                                ^                       |
                                                |                       v
                                    TC6 quadrature count <- motor + encoder

Joystick Y (CAN) -> servo position -> 50 Hz servo PWM
Fire button (CAN) -> solenoid pulse
```

The **SAM3X8E (Node 2)** controls the motor, servo, and solenoid. The **ATmega162 (Node 1)** reads the joystick and sends commands over CAN.

## DC motor: PWM and direction (`motor.c`)

- The motor shield accepts **PHASE/DIR** (`PC23`, D7) and **ENABLE/PWM** (`PB12`, D20, PWM channel 0).
- PWM uses the **84 MHz master clock** and a period count of **4200**: `84 MHz / 4200 = 20 kHz`.
- The sign of `motor_set_speed(percent)` controls the direction pin; the magnitude determines PWM duty cycle: `duty = 4200 * abs(percent) / 100`.
- `MOTOR_MAX_PCT = 50`: commands are clamped to **±50%**; `MOTOR_INVERT = 1` reverses the chosen direction convention.
- `motor_stop()` sets PWM duty to zero. `motor_init()` drives the enable signal low **before** assigning the pin to PWM, avoiding unintended startup movement.

**Important:** The motor driver's `percent` is a **PWM command**, *not a measured speed in rpm*. No inner speed-feedback loop is implemented here.

## Encoder-based PI position control (`controller.c`)

The incremental encoder is decoded in **TC2 channel 0 / TC6** hardware (`encoder.c`). The controller is called nominally every **10 ms (100 Hz)** from `main.c`.

```c
float e = (float)(ref - pos) * 100.0f / track_counts;
float p = KP * e;                  // KP = 2.0
float i = KI * integral;           // KI = 1.0
float u = p + i;                   // signed motor command
```

- `ref` and `pos` are **encoder counts**. Error is normalized to **percent of calibrated track length**.
- `integral += e * T` with `T = 0.010 s`, when output is not saturated.
- Output is clamped to **±50%**, matching the motor driver limit.
- **Basic anti-windup:** integration is paused when the computed output is outside the allowed range (not a complete anti-windup scheme).
- The joystick X value (`-100..100`) maps to position reference with **5% travel margins** at both ends.

## Track calibration (`main.c`)

1. Drive toward one end stop at command **−30%**, detecting when encoder movement ceases.
2. Reset the encoder counter to zero.
3. Drive toward the opposite end stop at **+30%**; the final count is `pos_max`.
4. Reject calibration if `pos_max < 5000` counts; without valid calibration the position-control loop stays disabled.

Stall detection: less than **5 counts** of movement for **300 ms**, with a **5 s timeout** per drive. This is *encoder-motion-based* stall detection, **not a current sensor**. Physically driving into the end stops requires care.

## Servo and solenoid

- **Servo (`servo.c`):** Joystick Y maps from `-100..100` to **900–2100 µs** pulses, with centre at **1500 µs**. The PWM period is **20 ms (50 Hz)** on `PB13`, PWM channel 1.
- **Solenoid (`solenoid.c`):** A button press triggers a **50 ms** GPIO pulse on `PB25` (active low in this configuration). The code enforces a **300 ms cooldown** and ends the pulse via `solenoid_update()` in the main loop (non-blocking).
- **CAN safety:** If joystick messages stop for **500 ms**, Node 2 centres the servo, stops the motor, and resets the PI controller.

## Oral-exam checkpoints

- **Why PWM?** Adjusts average electrical drive without generating a variable analog supply; direction is controlled separately.
- **What does the PI controller regulate?** **Position**, using encoder feedback; its output is a PWM/direction command.
- **Why the integral term?** Helps remove steady-state position error; excessive accumulation is limited with basic anti-windup.
- **Why calibrate?** An incremental encoder provides only relative counts after power-up.
- **Why the 500 ms timeout?** Prevents the motor continuing under stale joystick commands when CAN communication is lost.

*Source: supplied TTK4155 project ZIP, Node 2 implementation. See `encoder.md` for quadrature decoding and `overview.md` for a consolidated table of timing values.*
