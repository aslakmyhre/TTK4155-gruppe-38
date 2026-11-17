# External SRAM and Memory Mapping (Node 1)

## Overview

Node 1 uses the **ATmega162 external memory interface (XMEM)** to connect both an external **SRAM chip** and the **MAX156 ADC**. The SRAM is **static RAM**: volatile read/write memory, separate from the ATmega162's internal SRAM.

The lab manual's *Exercise 2* first suggests a **2 KiB** external SRAM region, then offers an optional **3 KiB** configuration. **Our project uses the 3 KiB configuration.** The SRAM IC itself has more capacity; only part is decoded and accessible in this setup.

## Logic Gate Implementation

Using the 4-nand ic we made

$$
A\lor B=\overline{\overline A\land\overline B}
$$

```
A ──┬──[NAND]── NOT A ──┐
    └──┘                │
                        [NAND]── A OR B
    ┌──┐                │
B ──┴──[NAND]── NOT B ──┘
```

| A11 | A10 | Unit |
|---|---|---|
| 0 | 0 | ADC |
| 0 | 1 | SRAM |
| 1 | 0 | SRAM |
| 1 | 1 | SRAM |


## Our memory map (`node1/xmem.h`)

| Address range | Device | Allocated range |
|---|---|---:|
| `0x1000–0x13FF` | MAX156 ADC (memory-mapped I/O) | 1 KiB |
| `0x1400–0x1FFF` | External SRAM | **3 KiB** |

```c
#define ADC_BASE  0x1000
#define ADC_SIZE  0x0400
#define SRAM_BASE 0x1400
#define SRAM_SIZE 0x0C00
```

The **ADC does not contain 1 KiB of RAM**: the allocated addresses select a peripheral. The mapping is deliberately simple rather than space-efficient.

## How XMEM works

- `PA0–PA7` carry the **low address byte and data** at different times (multiplexing).
- `PC0–PC7` provide upper address bits, although upper pins are masked for JTAG.
- The **74ALS573 address latch**, controlled by `ALE`, holds the low address byte while the bus is used for data.
- `RD` and `WR` indicate read/write cycles; the **address decoder** selects either SRAM or ADC using address lines and logic gates (the optional mapping uses A10/A11).

`node1/xmem.c` enables the interface:

```c
void xmem_init(void) {
    SFIOR |= (1 << XMM2); // Mask upper address pins for JTAG
    MCUCR |= (1 << SRE);  // Enable external memory interface
}
```

## Accessing the devices from C

```c
volatile uint8_t *ram = (uint8_t *)SRAM_BASE;
ram[0] = 42;               // External SRAM write
uint8_t x = ram[0];        // External SRAM read

volatile uint8_t *adc = (uint8_t *)ADC_BASE;
adc[0] = 0;               // Trigger ADC conversion (WR)
uint8_t sample = adc[0];   // ADC read (RD), not a RAM read
```

The CPU issues ordinary data-memory access instructions; **external hardware decides what responds**. `volatile` prevents compiler optimizations from removing or merging these accesses.

## How our code uses the SRAM

- `main.c` calls `xmem_init()` and then `sram_test()` during startup.
- `sram.c` tests the whole mapped **3 KiB** by writing pseudorandom values and reading them back, then reporting errors over UART.
- The code does **not** appear to configure AVR-GCC to use this external SRAM automatically as a general heap/stack extension. The explicit pointer accesses in `sram_test()` are different from expanding the default C runtime RAM.

## Oral-exam essentials

1. **Why a latch?** The low address bits and data share MCU pins; the latch preserves the address during data transfer.
2. **Why address decoding?** SRAM and ADC share a bus, so only the selected device should respond.
3. **Why 3 KiB instead of 2 KiB?** The optional lab design decodes two address bits, allocating more of the available address range to SRAM.
4. **Why is the ADC 'memory-mapped'?** Reading/writing a specific address causes ADC hardware actions rather than normal RAM storage.
5. **SRAM vs Flash/EEPROM:** SRAM is fast, writable and **volatile**; it loses content without power.

*Sources: TTK4155 lab manual v6.1.5, Exercise 2 (pp. 29–33); `node1/xmem.[ch]`, `node1/sram.c`, `node1/adc.c`, `node1/main.c`.*
