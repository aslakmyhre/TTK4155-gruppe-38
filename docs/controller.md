# PI Position Controller — Node 2 (TTK4155)

Short study note based on `node2/controller.c`, `controller.h`, `main.c`, and `motor.c`.

## What are we controlling?

The joystick's **X position** sets a *target position* for the racket, not a motor speed. The motor moves until encoder feedback approaches that target.

```text
Joystick X → target (encoder counts) → [ PI controller ] → signed PWM command → motor
                                           ↑                              |
                                           └──────── encoder counts ───────┘
```

The controller is called approximately every **10 ms (100 Hz)** while calibration is valid and joystick communication is active.

## Actual implementation (`controller.c`)

```c
#define KP 2.0f
#define KI 1.0f
#define T (CONTROLLER_PERIOD_MS / 1000.0f)  // 0.010 s

float e = (float)(ref - pos) * 100.0f / track_counts;
float p = KP * e;
float i = KI * integral;
float u = p + i;
```

- `ref` = desired position, `pos` = measured encoder position.
- `track_counts` = calibrated track length in encoder counts.
- `e` = position error **as a percentage of track length**, not raw counts.
- **P term (`p`)** reacts to the current error; larger error gives larger motor command.
- **I term (`i`)** reacts to accumulated past error; helps overcome steady-state offsets (e.g., friction).
- `u` is a **signed PWM percentage command**, not a measured speed in rpm.

For example, a 10%-of-track error produces `p = 2.0 × 10 = 20` percentage points of motor command before adding the I term.

## Integral update and limits

```c
if (u > U_MAX)
    u = U_MAX;
else if (u < -U_MAX)
    u = -U_MAX;
else
    integral += e * T;
```

- `U_MAX` equals `MOTOR_MAX_PCT` (**50%** in this project).
- Output is clamped to **−50%…+50%**.
- **Simple anti-windup:** the integral is *not increased* when the **currently computed** output exceeds the limit.
- Subtle detail: `i` and `u` are calculated **before** updating `integral`, so the new integral contributes to the *next* update.
- This is basic conditional integration, not a full anti-windup solution. It may still keep accumulated error longer than desired.

## From reference to physical movement (`main.c`, `motor.c`)

- Joystick X (`−100…+100`) is mapped to encoder target counts with a **5% margin at each end**.
- On startup, the motor is driven toward each end stop to determine `pos_max`; then `controller_init(pos_max)` stores the track length.
- `motor_set_speed(controller_update(ref, encoder_read()))` applies the PI output.
- `motor_set_speed()` maps magnitude to PWM duty and sign to a direction pin.
- The motor PWM is **20 kHz**, while the *PI loop* is **100 Hz**. These are different frequencies.
- If joystick CAN messages are missing for **500 ms**, the motor stops and `controller_reset()` clears the accumulated integral.

## Why use feedback?

Without position feedback, a fixed PWM command cannot reliably put the racket at a specified location: load and friction affect movement. Encoder measurements let the controller correct its command as the target is approached.

## Oral-exam essentials

**Why P?** Responds immediately to position error.  
**Why I?** Helps remove a persistent error that P-only control might leave.  
**Why clamp `u`?** The motor command is limited to ±50%.  
**Why anti-windup?** To avoid building up a large integral while output is saturated.  
**Is `u` motor speed?** No: it is a PWM/direction *command*, not measured RPM.  
**What do 100 Hz and 20 kHz refer to?** PI update rate versus hardware PWM switching frequency.

**Further question to investigate:** How do `KP` and `KI` affect overshoot, settling time and oscillation on the actual racket?
