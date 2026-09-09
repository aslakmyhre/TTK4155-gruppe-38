#include "sram.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <avr/io.h>

void sram_init(void) {
    // JTAG owns PC2-PC5, which the memory interface needs for A10-A13.
    // The JTD bit must be written twice within four cycles.
    MCUCSR |= (1 << JTD);
    MCUCSR |= (1 << JTD);

    // SRW10 adds one wait state to the whole external memory space. The
    // address latch plus SRAM is usually too slow for zero wait states.
    MCUCR |= (1 << SRE) | (1 << SRW10);
}

void sram_test(void) {
    volatile char *ext_ram = (char *) 0x1800; // Start address for the SRAM
    uint16_t ext_ram_size = 0x800;
    uint16_t write_errors = 0;
    uint16_t retrieval_errors = 0;
    printf("Starting SRAM test...\n");

    // rand() stores some internal state, so calling this function in a loop will
    // yield different seeds each time (unless srand() is called before this function)
    uint16_t seed = rand();

    // Write phase: Immediately check that the correct value was stored
    srand(seed);
    for (uint16_t i = 0; i < ext_ram_size; i++) {
        if ((i & 0xFF) == 0) {
            putchar('.'); // progress, so a slow or hanging test is visible
        }
        uint8_t some_value = rand();
        ext_ram[i] = some_value;
        uint8_t retreived_value = ext_ram[i];
        if (retreived_value != some_value) {
            printf("Write phase error: ext_ram[%4d] = %02X (should be %02X)\n",
                   i, retreived_value, some_value);
            write_errors++;
        }
    }

    // Retrieval phase: Check that no values were changed during or after the write phase
    srand(seed); // reset the PRNG to the state it had before the write phase
    for (uint16_t i = 0; i < ext_ram_size; i++) {
        uint8_t some_value = rand();
        uint8_t retreived_value = ext_ram[i];
        if (retreived_value != some_value) {
            printf("Retrieval phase error: ext_ram[%4d] = %02X (should be %02X)\n",
                   i, retreived_value, some_value);
            retrieval_errors++;
        }
    }

    printf("SRAM test completed with \n%4d errors in write phase and \n%4d errors in retrieval phase\n\n",
           write_errors, retrieval_errors);
}

void sram_address_test(void) {
    volatile char *ext_ram = (char *) 0x1800;
    const uint8_t highest_bit = 10; // 0x800 bytes -> A0..A10

    printf("Starting address line test...\n");

    // Each power-of-two address gets a unique marker. If two address lines are
    // shorted, or one never reaches the SRAM, two markers share a cell and the
    // earlier one is overwritten.
    ext_ram[0] = 0xAA;
    for (uint8_t bit = 0; bit <= highest_bit; bit++) {
        ext_ram[(uint16_t) 1 << bit] = bit;
    }

    uint8_t base = ext_ram[0];
    if (base != 0xAA) {
        printf("A%d aliases address 0: ext_ram[0] = %02X\n", base, base);
    }

    for (uint8_t bit = 0; bit <= highest_bit; bit++) {
        uint8_t value = ext_ram[(uint16_t) 1 << bit];
        if (value != bit) {
            printf("A%-2d dead or aliased: ext_ram[%04X] = %02X (should be %02X)\n",
                   bit, (uint16_t) 1 << bit, value, bit);
        }
    }

    printf("Address line test completed\n\n");
}
