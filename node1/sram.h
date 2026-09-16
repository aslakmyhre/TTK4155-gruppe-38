#ifndef SRAM_H
#define SRAM_H

// Enables the external memory interface so the SRAM is mapped into the
// data address space. Must be called before any access to ext_ram.
void sram_init(void);

// Writes and reads back the whole SRAM, reporting errors over UART.
void sram_test(void);

#endif
