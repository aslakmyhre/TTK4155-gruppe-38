#ifndef SRAM_H
#define SRAM_H

// Writes and reads back the whole SRAM, reporting errors over UART.
// Requires xmem_init() to have been called.
void sram_test(void);

#endif
