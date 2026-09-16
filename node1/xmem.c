#include "xmem.h"

#include <avr/io.h>


void xmem_init(void) {
    // Masking PC4-PC7 out of the address bus leaves them to JTAG, so the
    // Atmel-ICE can stay connected and debugging keeps working.
    SFIOR |= (1 << XMM2);
    MCUCR |= (1 << SRE);
}
