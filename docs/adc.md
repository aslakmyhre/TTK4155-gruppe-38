# ADC — Quick Notes (TTK4155 Node 1)

## Hardware and memory mapping

- Node 1 uses an **external MAX156 8-bit ADC** with four analog inputs (AIN0–AIN3), rather than an ADC inside the ATmega162.
- The ADC is connected to the **external memory interface (XMEM)**, not SPI. `xmem_init()` must run before accessing it.
- `ADC_BASE` is `0x1000` (`node1/xmem.h`). The driver accesses it through a volatile pointer:

```c
static volatile uint8_t *const adc = (uint8_t *)ADC_BASE;
```

Writing and reading that address causes **external bus transactions**, not ordinary RAM accesses.

## Clock and conversion (`node1/adc.c`)

- Timer1 toggles **OC1A / PD5** to provide the MAX156's external clock.
- With `OCR1A = 0` and no prescaler: `f_ADC_CLK = F_CPU / 2 = 2.4576 MHz` (for `F_CPU = 4.9152 MHz`).
- The code starts a conversion with a **write** to `adc[0]`, then waits using `_delay_us(...)` because the ADC's **BUSY** pin is not connected.

```c
void adc_read(uint8_t values[ADC_NUM_CHANNELS]) {
    adc[0] = ADC_START_CONVERSION;
    _delay_us(ADC_CONVERSION_US);
    for (uint8_t channel = 0; channel < ADC_NUM_CHANNELS; channel++) {
        values[channel] = adc[0];
    }
}
```

**Why read the same address four times?** Each external read advances the ADC's internal read pointer to the next channel; it is *not* four reads from a static RAM location. A write starts conversion and resets the channel read sequence.

The code conservatively allows conversion time for **eight channels** (`ADC_CONVERTED_CHANNELS = 8`), based on its interpretation of the datasheet's hard-wired MODE timing.

## Channel mapping (`node1/calibration.h`)

| Channel | Signal |
|---|---|
| 0 | Touchpad X |
| 1 | Touchpad Y |
| 2 | Joystick X |
| 3 | Joystick Y |

## Oral-exam checkpoints

- Why use XMEM instead of SPI? How does **address decoding** select the ADC?
- Why is the ADC pointer `volatile`?
- Why does the ADC need an external clock, and how does Timer1 generate it?
- Why is there a delay between starting conversion and reading results?
- Why do repeated reads of `adc[0]` return different channels?

*Source: `node1/adc.c`, `adc.h`, `xmem.h`, `calibration.h`.*
