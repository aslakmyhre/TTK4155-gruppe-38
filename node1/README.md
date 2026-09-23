# TTK4155 Node 1

Starter code for node 1 (ATmega162) in TTK4155 Embedded and Industrial Computer Systems Design at NTNU. This repository is *Linux-only* and uses `avr-gcc` and Linux tooling for debugging. 

# Requirements
- avr toolchain

# Picocom
`picocom -b 9600 /dev/ttyS0`

See Makefile for commands to build, flash, debug etc. 


# Joystick request experiment

The default program sends only AVR command `0x03` (joystick read) and prints
the three raw response bytes as X, Y and button over UART at 9600 baud, 8N1.
It waits five seconds before the first request so you can observe the OLED
before polling starts, then reads roughly every half second. It sends no
OLED commands, LED commands, or other AVR commands. PB0/PB3 (OLED D/C/reset)
remain inputs after MCU reset; SPI initialization does not configure them.

This tests whether the board firmware updates the OLED as a side effect of
a joystick request. That behavior is not guaranteed by the board protocol.

## Wiring

| ATmega162 / supply | IO-board signal | PDF page 3 header label |
| --- | --- | --- |
| PB1 | DISP_CS (held inactive/high) | 1 |
| PB2 | IO_CS | 2 |
| PB5 / MOSI | MOSI | 3 |
| PB6 / MISO | MISO | 4 |
| PB7 / SCK | SCK | 5 |
| Common GND | GND | 7 |
| 5 V supply | 5V | 8 |

Leave PB0 (OLED D/C) and PB3 (OLED reset) disconnected for this test.
Ensure DISP_RES is held high using the board's reset-to-5V jumper JP1 or
the board manual's 5V connection. Do not leave reset floating. Reconnect
JP1 only after disconnecting any MCU connection to DISP_RES. The PDF's header labels are not the
schematic's 1–14 connector pin numbers; use its orientation diagram.

## Running the experiment

From `node1`:

```sh
make
make flash
picocom -b 9600 /dev/ttyS0
```

Power-cycle both boards after flashing, so the earlier OLED test's image
and initialization do not carry over. Observe the OLED during the initial
five-second pause, then while joystick polling runs. Move and press the
joystick and check that the UART values respond. The initial UART message
may be missed if the terminal is opened after startup.

An OLED change when polling begins supports the hypothesis; an unchanged
display with responsive UART values means this experiment did not trigger
an automatic display update. Constant 00/FF response bytes do not establish
working communication: check power, wiring, chip select, and SPI settings.
SPI mode 0, MSB first and 38.4 kHz remain assumptions to verify on hardware.
The required 40 us command-to-data and 2 us read-data gaps are retained.

## Other examples

The previous full IO-board/display test is saved as
`examples/io_board_full_demo.c`. The OLED driver is retained but excluded
from the current build; its own initialization now configures D/C and reset.

The original MAX156 ADC application is saved as `examples/adc_demo.c`.
This joystick experiment is separate from exercise 3.3.4's ADC measurements.
The original ADC plotter documentation is in `tools/README.md`; this
experiment's UART output is not in its plotting format.

PB4 is reserved as an output held high: it is the ATmega162 hardware SS
pin even though IO_CS is on PB2. Do not connect PB4 to a source driving it
low. A local SPI failure now prints register values and stops instead of
waiting forever; no response bytes from that failed read are printed.
