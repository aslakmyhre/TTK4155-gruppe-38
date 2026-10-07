# TTK4155 Node 2

Starter code for node 2 in TTK4155 Embedded and Industrial Computer Systems Design at NTNU. This repository contains two kinds of files:

## Cross-platform starter code (Linux and Windows)

- `can.c` / `can.h`    CAN driver
- `time.c` / `time.h`  timing helpers
- `uart.c` / `uart.h`  UART driver + printf support

## Linux-only build environment

- `main.c`   example entry point
- `Makefile` arm-none-eabi-gcc build + openocd flashing
- `sam/`     CMSIS headers, startup code, linker script, openocd config

## Windows users (Atmel Studio)

Ignore `Makefile`, `main.c` and `sam/`. Copy `can.*`, `time.*` and `uart.*`

