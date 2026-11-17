# User Interface — Quick Notes (TTK4155 Node 1)

**Scope:** `calibration.c`, `joystick.c`, `menu.c` (plus their headers). ADC hardware is covered separately in [`adc.md`](adc.md); OLED and SPI have separate notes.

## Data flow

```text
Joystick/touchpad voltages
    -> MAX156 ADC (raw bytes, 0–255)
    -> calibration_apply() (normalized 0–255, center = 128)
    -> joystick_position() (X/Y approximately -100% to +100%)
    -> joystick_direction() -> menu events / application control
```

## Calibration (`node1/calibration.c`)

Each axis stores three values:

```c
struct axis_calibration { uint8_t low, center, high; };
```

- User follows **OLED prompts**, confirming readings with I/O-board buttons.
- Joystick: measure **rest/center**, then both extremes on each axis.
- Touchpad: measure extremes; calculate center as their midpoint.
- Reject calibration if center is not between the extremes.
- `calibration_apply()` **scales the two halves independently**: low → 0, center → 128, high → 255; results are clamped to 0–255. This also handles inverted raw directions.
- Saved in **EEPROM** so calibration survives reset/power-off. A `0xA5` marker identifies a completed save; the marker is invalidated during writing and restored last.
- `calibration_ask()` loads EEPROM values or defaults (`0/128/255`), then offers **calibrate / skip** at startup.

## Joystick (`node1/joystick.c`, `joystick.h`)

- `joystick_position(x, y)` maps calibrated bytes to signed percentage values.
- `JOYSTICK_DEADZONE_PERCENT = 20`: if both axes are within the deadzone, direction is `NEUTRAL`.
- Otherwise the axis with the **largest absolute deflection** determines `LEFT`, `RIGHT`, `UP`, or `DOWN`.
- These direction values are convenient for menus; X/Y percentages are also useful as continuous control inputs.

## Menu (`node1/menu.c`)

- Menu structure contains a **title, list of items and count**. Each item may link to a submenu or an action ID.
- `menu_run()` **blocks** until a leaf item is selected and returns its action.
- Input is polled every **20 ms** (`MENU_POLL_MS`).
- **UP/DOWN:** change selection (wrap around); **LEFT:** back; **RIGHT:** open submenu; **click:** open submenu or select a leaf.
- `to_event(now, before)` only reacts to **changes** in direction/button state, so holding the stick doesn't repeatedly navigate.
- Up to **four nested menu levels** (`MENU_MAX_DEPTH = 4`).
- `move_selection()` normally redraws only affected OLED lines; a scroll requires a larger redraw. This matters because the configured SPI is slow.

## How it connects to the game

`node1/main.c` reads ADC values, applies calibration and turns them into joystick positions. The game loop uses `GAME_PERIOD_MS = 50` as its delay/nominal update interval, and sends control information to Node 2 via CAN.

## Oral-exam checkpoints

- Why calibrate each axis using *low, center, high* rather than one fixed linear scale?
- Why save values in EEPROM, and why write the marker **last**?
- What does the deadzone prevent? Why pick the axis with larger deflection?
- Why does the menu compare current input against previous input?
- Why redraw only changed menu lines?

*Source: `node1/calibration.c/.h`, `joystick.c/.h`, `menu.c/.h`, `main.c`.*
