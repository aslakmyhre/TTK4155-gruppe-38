#include "image.h"
#include "oled.h"
#include "spi.h"

int main(void)
{
    spi_init();
    oled_init();

    /* 64x64 image centered: (128 - 64) / 2 = 32 */
    oled_draw_image_h(32, 0, IMAGE_WIDTH, IMAGE_HEIGHT, image_bitmap);

    while (1) { }
}
