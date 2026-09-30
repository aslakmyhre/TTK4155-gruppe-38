#ifndef JOYSTICK_BOX_H
#define JOYSTICK_BOX_H

#include <stdint.h>
#include "joystick.h"

// A framed square on the OLED with a dot at the joystick position
#define JOYSTICK_BOX_SIZE       40   /* pixels, square */
#define JOYSTICK_BOX_FIRST_PAGE 1    /* covers display pages 1-5 */

struct joystick_box {
    uint8_t x;       /* left column on the display */
    uint8_t dot_x;   /* top-left pixel of the dot, inside the box */
    uint8_t dot_y;
};

// Draws the empty box with the dot at rest, left edge at column x.
void joystick_box_init(struct joystick_box *box, uint8_t x);

// Moves the dot. Only the pages the dot left or entered are redrawn: at the
// SPI clock a full box takes ~40 ms.
void joystick_box_show(struct joystick_box *box, struct joystick_position pos);

#endif
