#include "xmem.h"

#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>
#include "uart.h"

// First and last address of each device, where decoding errors show up
static const uint16_t decode_test_addresses[] = {
    ADC_BASE,
    ADC_BASE + ADC_SIZE - 1,
    SRAM_BASE,
    SRAM_BASE + SRAM_SIZE - 1,
};


void xmem_init(void) {
    // Masking PC4-PC7 out of the address bus leaves them to JTAG, so the
    // Atmel-ICE can stay connected and debugging keeps working.
    SFIOR |= (1 << XMM2);
    MCUCR |= (1 << SRE);
}

static void read_until_key(uint16_t address) {
    // volatile forces a real bus read (RD pulse) on every iteration
    volatile uint8_t *location = (uint8_t *) address;
    while (!uart_has_data()) {
        (void) *location;
    }
    uart_receive();
}

void xmem_decode_test(void) {
    const uint8_t count = sizeof decode_test_addresses / sizeof decode_test_addresses[0];
    for (uint8_t i = 0; i < count; i++) {
        uint16_t address = decode_test_addresses[i];
        const char *expected = address < SRAM_BASE ? "ADC" : "SRAM";
        printf("Reading 0x%04X, expect %s CS active. Key = next\n", address, expected);
        read_until_key(address);
    }
    printf("Decode test done\n");
}
