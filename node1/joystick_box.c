#include <stdbool.h>
#include "joystick_box.h"
#include "oled.h"

#define PAGE_HEIGHT  8
#define BOX_PAGES    (JOYSTICK_BOX_SIZE / PAGE_HEIGHT)
#define BORDER_LAST  (JOYSTICK_BOX_SIZE - 1)
#define CENTER       (JOYSTICK_BOX_SIZE / 2)
#define DOT_SIZE     4
#define DOT_TRAVEL   (JOYSTICK_BOX_SIZE - 2 - DOT_SIZE)   /* stays inside the border */
#define PERCENT_MAX  100

/* -100..100 percent to the dot's first pixel, 1..DOT_TRAVEL+1. Received
   positions can hold any int8_t, and past the ends the dot would be drawn
   outside the box, so it is held at the edge. */
static uint8_t dot_offset(int16_t percent)
{
    if (percent < -PERCENT_MAX)
        percent = -PERCENT_MAX;
    if (percent > PERCENT_MAX)
        percent = PERCENT_MAX;
    return (uint8_t)(1 + (percent + PERCENT_MAX) * DOT_TRAVEL / (2 * PERCENT_MAX));
}

static bool is_lit(const struct joystick_box *box, uint8_t col, uint8_t row)
{
    bool border = col == 0 || col == BORDER_LAST || row == 0 || row == BORDER_LAST;
    bool center = col == CENTER && row == CENTER;
    bool dot = col >= box->dot_x && col < box->dot_x + DOT_SIZE
            && row >= box->dot_y && row < box->dot_y + DOT_SIZE;
    return border || center || dot;
}

static void draw_page(const struct joystick_box *box, uint8_t page)
{
    uint8_t columns[JOYSTICK_BOX_SIZE];
    for (uint8_t col = 0; col < JOYSTICK_BOX_SIZE; col++) {
        uint8_t byte = 0;
        for (uint8_t bit = 0; bit < PAGE_HEIGHT; bit++) {
            if (is_lit(box, col, page * PAGE_HEIGHT + bit))
                byte |= 1 << bit;
        }
        columns[col] = byte;
    }
    oled_draw_columns(box->x, JOYSTICK_BOX_FIRST_PAGE + page, columns, JOYSTICK_BOX_SIZE);
}

void joystick_box_init(struct joystick_box *box, uint8_t x)
{
    box->x = x;
    box->dot_x = dot_offset(0);
    box->dot_y = dot_offset(0);
    for (uint8_t page = 0; page < BOX_PAGES; page++)
        draw_page(box, page);
}

void joystick_box_show(struct joystick_box *box, struct joystick_position pos)
{
    uint8_t old_y = box->dot_y;
    uint8_t new_x = dot_offset(pos.x);
    uint8_t new_y = dot_offset(-pos.y);   /* up is positive, screen rows grow down */
    if (new_x == box->dot_x && new_y == old_y)
        return;
    box->dot_x = new_x;
    box->dot_y = new_y;

    uint8_t top = old_y < new_y ? old_y : new_y;
    uint8_t bottom = (old_y > new_y ? old_y : new_y) + DOT_SIZE - 1;
    for (uint8_t page = top / PAGE_HEIGHT; page <= bottom / PAGE_HEIGHT; page++)
        draw_page(box, page);
}
