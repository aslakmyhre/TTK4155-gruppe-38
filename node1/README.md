# TTK4155 Node 1

Starter code for node 1 (ATmega162) in TTK4155 Embedded and Industrial Computer Systems Design at NTNU. This repository is *Linux-only* and uses `avr-gcc` and Linux tooling for debugging. 

# Requirements
- avr toolchain

# Picocom
`picocom -b 9600 /dev/ttyS0`

See Makefile for commands to build, flash, debug etc. 


# ADC plotter
Live plot of the joystick and touchpad positions over UART: see `tools/README.md`.
