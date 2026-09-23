#include <util/delay.h>
#include "image.h"
#include "oled.h"
#include "spi.h"

#define IMAGE_X      32   /* 64x64 image centered: (128 - 64) / 2 */
#define IMAGE_PAGE   0    /* the image fills the full height */
#define LOAD_STEP_MS 15   /* pause per pixel row, sets the loading speed */

int main(void)
{
    spi_init();
    oled_init();

    /* Reveal the image one pixel row at a time, from the bottom up. Only the
       page holding the new row changes, so only that page is redrawn. */
    for (int8_t row = IMAGE_HEIGHT - 1; row >= 0; row--) {
        uint8_t p = row / 8;
        uint8_t mask = 0xFF << (row % 8);   /* this row and the ones below it */
        oled_draw_image_h_page(IMAGE_X, IMAGE_PAGE + p, IMAGE_WIDTH,
                               image_bitmap, p, mask);
        _delay_ms(LOAD_STEP_MS);
    }

    while (1) { }
}
