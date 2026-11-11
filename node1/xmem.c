#include "xmem.h"

#include <avr/io.h>


void xmem_init(void) {
    //Disable PC4:PC7, whos pins are used by jtag
    SFIOR |= (1 << XMM2);
    //SRAM enable
    MCUCR |= (1 << SRE);
}
