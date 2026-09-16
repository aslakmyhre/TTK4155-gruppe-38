#ifndef XMEM_H
#define XMEM_H

// Memory map, decoded from A10/A11 (A12-A15 are masked out for JTAG)
#define ADC_BASE  0x1000
#define ADC_SIZE  0x0400
#define SRAM_BASE 0x1400
#define SRAM_SIZE 0x0C00

// Enables the external memory bus. Must be called before accessing
// any memory-mapped device.
void xmem_init(void);

// Reads each decoder boundary address repeatedly so its chip select can be
// checked on the scope. A key press moves on to the next address.
void xmem_decode_test(void);

#endif
