#ifndef SRAM_H
#define SRAM_H

// Enables the external memory interface so the SRAM is mapped into the
// data address space. Must be called before any access to ext_ram.
void sram_init(void);

// Writes and reads back the whole SRAM, reporting errors over UART.
void sram_test(void);

// Walking-ones check of the address lines. Names the address bits that are
// stuck or aliased, which sram_test() can only show as a bulk error count.
void sram_address_test(void);

#endif
