# OLED — Quick Notes (Node 1)

**Hardware:** 128 × 64 monochrome OLED (SSD1309), controlled by the ATmega162 over SPI.  
**Code:** `node1/oled.c`, `oled.h`, `fonts.h`, `spi.c`.

## 1. Display memory: pages and columns

- The display is **128 columns × 64 rows**, organized as **8 pages of 8 vertical pixels**.
- Writing **one data byte** at a page/column controls **8 vertically adjacent pixels**; bit 0 is the top pixel in the project's column format.
- A full screen represents **128 × 8 = 1024 display-data bytes**.
- `oled_pos(page, column)` selects where the next data byte goes (`page` 0–7, `column` 0–127).

## 2. Commands versus pixel data


The OLED shares the SPI bus with other devices, but has its own **CS** pin. 

### Structure

The OLED-driver uses the following structure.

```
Application (main.c)
        │
        ▼
 oled_print()
 oled_putchar()
 oled_clear()
 oled_pos()
        │
        ▼
  oled_cmd()
  oled_data()
        │
        ▼
  spi_transfer()
        │
        ▼
      SSD1309
        │
        ▼
   OLED pixels
```

Most of the communication happens via `oled_cmd()` and `oled_data()`. This happens by using an additional **D/C** pin tells it what the transmitted byte means.

- **Command:** configure the screen or set its address/cursor.
- **Data:** write a byte of pixel information into display RAM.
- The driver uses `oled_cmd()` and `oled_data()` to hide these details.

For instance

```c
static void oled_cmd(uint8_t c)
{
    PORTB &= ~(1 << SPI_DISPLAY_DC);   // command mode
    spi_select(SPI_SLAVE_DISPLAY);
    (void)spi_transfer(c);
    spi_deselect_all();
}
```

Here we set `PORTB`'s SPI display pin `LOW` (by using a bitmask), this way the OLED controller know how to interpret the byte we send over SPI.

### oled_data()

This function is used for sending data directly to the OLED's screen memory.

```c
static void oled_data(uint8_t d)
{
    PORTB |= (1 << SPI_DISPLAY_DC);

    spi_select(SPI_SLAVE_DISPLAY);
    (void)spi_transfer(d);
    spi_deselect_all();
}
```

E.g.

```c
oled_data(0b11111111);
```
Will turn on **8 pixels** in a vertical columns **on**. Where as

```c
oled_data(0b00000000);
```
Will configure the same pixels to **off**. The location is decided by the SSD1309's write position. The **`oled_data()-command` increments the column if there are sequential commands written**. I.e.

```c
oled_pos(0, 0);  // Page 0, column 0

oled_data(0b00000001);
oled_data(0b00000010);
oled_data(0b00000100);
```

Will draw

```
          Column
          0  1  2

Row 0     █  ·  ·
Row 1     ·  █  ·
Row 2     ·  ·  █
Row 3     ·  ·  ·
Row 4     ·  ·  ·
Row 5     ·  ·  ·
Row 6     ·  ·  ·
Row 7     ·  ·  ·
```

Lastly, the use of `pgm_read_byte(&glyph[i])` is used to get the font-table from flash memory on the `Atmel162`. This gets us **5 byte** that represent the line width and character pixels.


## 3. Initialization and positioning

`oled_init()` waits for power-up, turns the display off (`0xAE`), sets orientation and **page addressing mode**, clears display RAM, then turns it on (`0xAF`).

- `oled_goto_line(line)` sends `0xB0 | line` to select a page.
- `oled_goto_column(column)` sends two commands for the **low and high column-address nibbles**.
- `oled_clear_line()` writes 128 zero bytes to one page; `oled_clear()` does this for all 8 pages.
- `oled_contrast(level)` sends `0x81` followed by a contrast value.

## 4. Drawing text

`oled_putchar(c)` uses the **5-column font** in `fonts.h` (stored in AVR flash/`PROGMEM`):

1. Find the character's glyph.
2. Read each of its five column bytes with `pgm_read_byte()`.
3. Send those bytes as OLED data, then one blank column for spacing.

`oled_print()` repeats this for a string. `oled_output()` provides a `FILE *` stream so `fprintf()` can also draw text; `\n` moves to the next page.

The `oled_goto_line()`-function is implemented as

```c
void oled_goto_line(uint8_t line)
{
    current_line = line % OLED_LINES;
    oled_cmd(0xB0 | current_line);
}
```

Here **bitwise or** is used to combine the OLED's command for selecting page (`0xB0`) with the lower 4 bit which is used to select the desired **page**. I.e.

```
0xB0 = 10110000
0x03 = 00000011
       ────────
OR   = 10110011 = 0xB3
```

This results in us selecting **page 3**, i.e. `oled_cmd(0xB3)` means, select **page 3**. Then for the column we use

```c
void oled_goto_column(uint8_t column)
{
    oled_cmd(0x00 | (column & 0x0F));
    oled_cmd(0x10 | (column >> 4));
}
```

Here `SSD1309` uses the **lower 4 bit** in conjunction with the **upper 4 bit** for selecting the column. Then we use


```c
void oled_putchar(char c)
{
    if (c < FONT_FIRST_CHAR || c > FONT_LAST_CHAR)
        c = '?';

    const uint8_t *glyph = font5[c - FONT_FIRST_CHAR];

    for (uint8_t i = 0; i < FONT_WIDTH; i++)
        oled_data(pgm_read_byte(&glyph[i]));

    oled_data(0x00);
}
```



## 5. Images and buffers

- `oled_draw_columns()` writes prearranged **vertical column bytes from RAM**.
- `oled_draw_image_h_page()` reads a bitmap stored in **horizontal-byte format in flash** and repacks pixels into the OLED's **vertical page-byte format** (`image_h_byte()`).
- The shown driver writes data **directly to the OLED**; it does **not** maintain a full 1024-byte framebuffer in `oled.c`.

### How the Screen Memory is Organized

The `SSD1309` organizes its memory into **8 pages**, where each page is **8 pixels** in height. 

```
        OLED DISPLAY (128 × 64)

        Columns 0 ... 127
        ┌─────────────────────────┐
Page 0  │ Rows  0–7               │
        ├─────────────────────────┤
Page 1  │ Rows  8–15              │
        ├─────────────────────────┤
Page 2  │ Rows 16–23              │
        ├─────────────────────────┤
Page 3  │ Rows 24–31              │
        ├─────────────────────────┤
Page 4  │ Rows 32–39              │
        ├─────────────────────────┤
Page 5  │ Rows 40–47              │
        ├─────────────────────────┤
Page 6  │ Rows 48–55              │
        ├─────────────────────────┤
Page 7  │ Rows 56–63              │
        └─────────────────────────┘
```

Each combination of page and columb correspond to **1 byte** in screen memory. E.g.

```c
oled_pos(2, 10);
oled_data(0b00001111);
```

Will select **page 2** (pixel row 16-23) and **column 10**, then write **8 bits** to that column.



## 6. Timing and design tradeoffs

SPI is configured at **38.4 kHz** in `spi.c`. Sending 1024 pixel-data bytes alone would take at least:

`1024 × 8 / 38400 ≈ 0.213 s`

Real updates take longer due to commands and software overhead. In this implementation, **CS is selected and deselected for each individual command/data byte**, which is simple but adds overhead. A faster SPI clock and/or batching transfers could improve display performance.




## Quick oral-exam checks

- Why does one OLED data byte represent **8 vertical pixels**, not 8 horizontal pixels?
- What is the difference between **CS** and **D/C**?
- How does `oled_putchar()` turn a character into pixels?
- Why is `pgm_read_byte()` used for fonts?
- Why might displaying a whole image be relatively slow at 38.4 kHz?
