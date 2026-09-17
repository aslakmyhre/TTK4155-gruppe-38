# ADC plotter

`plot_adc.py` reads the ADC lines node 1 prints on the UART and draws the
**live** position of the joystick and the touchpad while you move them.
It is not a log viewer: every frame comes from the serial port as the board
sends it, so the dot on screen follows your hand.

Two square panels, both scaled 0-255 (the full 8-bit range of the MAX156):

| Panel      | X axis        | Y axis        |
| ---------- | ------------- | ------------- |
| `joystick` | ADC channel 2 | ADC channel 3 |
| `touchpad` | ADC channel 0 | ADC channel 1 |

Each panel shows the current sample as a red dot, the last 60 samples as a
trail, a crosshair at the centre (128, 128) to judge the joystick's rest
position, and the raw values underneath. The dot is the *approximated*
position: it is the raw conversion result, with no calibration, deadzone or
scaling applied.

## Requirements

```
pip3 install pyserial matplotlib
```

The board must be flashed and running, and **nothing else may hold the serial
port** - close `picocom` first, otherwise the port is busy or the two programs
steal each other's bytes.

## Running

```
cd node1
./tools/plot_adc.py
```

The port is autodetected when exactly one USB serial device is present. When
there are several, or none is recognised, pass it:

```
./tools/plot_adc.py --port /dev/ttyUSB0          # Linux
./tools/plot_adc.py --port /dev/cu.usbmodem1101  # macOS
```

`--baud` defaults to 9600 and must match `uart_init(UBRR_VALUE(...))` in
`main.c`.

## Choosing which channel drives which axis

From the command line, `--joystick X,Y` and `--touchpad X,Y` take channel
numbers 0-3:

```
./tools/plot_adc.py --joystick 3,2 --touchpad 1,0
```

Or change it while the plot is running, with the plot window focused:

| Key   | Effect                              |
| ----- | ----------------------------------- |
| `j`   | next channel on the joystick X axis |
| `J`   | next channel on the joystick Y axis |
| `t`   | next channel on the touchpad X axis |
| `T`   | next channel on the touchpad Y axis |
| `c`   | clear both trails                   |

The panel title always states the mapping in use, e.g.
`joystick   X=ch2  Y=ch3`. Cycling a channel clears that panel's trail so the
old mapping is not left on screen.

Useful for finding out which channel a stick or touchpad axis is actually wired to:
wiggle one input, then cycle channels until the dot moves along the axis you
expect.

## Logging

```
./tools/plot_adc.py --csv joystick.csv
```

Writes every received sample as `seconds,ch0,ch1,ch2,ch3` while still plotting
live. The timestamp is host arrival time relative to the start of the run.

## Other options

| Option       | Default | Meaning                             |
| ------------ | ------- | ----------------------------------- |
| `--trail`    | 60      | samples kept in the trail           |
| `--interval` | 33      | redraw period in ms (~30 fps)       |

## Troubleshooting

- **`ignored line: ...` on stderr.** Lines without exactly four integers in
  0-255 are not samples. A few right after a reset are normal (half-printed
  line, `sram_test` output). A constant stream of them means the baud rate is
  wrong.
- **`no USB serial port found, pass --port`.** The adapter is not enumerated;
  check the cable, or name the port explicitly.
- **The dot moves in visible steps.** The firmware's `_delay_ms(200)` in
  `main.c` limits it to 5 samples per second. Lower the delay (and raise the
  baud rate to match) for a smooth trace.
