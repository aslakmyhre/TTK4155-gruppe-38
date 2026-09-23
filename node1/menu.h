#ifndef MENU_H
#define MENU_H

#include <stdint.h>
#include "adc.h"
#include "calibration.h"

// Returned by menu_run() when SPI fails; check spi_failed() for details
#define MENU_ACTION_SPI_FAILED 0

struct menu;

// A leaf returns its action (not MENU_ACTION_SPI_FAILED) when clicked; an
// item with a submenu opens it instead.
struct menu_item {
    const char *label;
    const struct menu *submenu;
    uint8_t action;
};

struct menu {
    const char *title;
    const struct menu_item *items;
    uint8_t count;   /* at least 1 */
};

// Draws the menu and blocks until a leaf is chosen, returning its action.
// Joystick UP/DOWN moves (one step per deflection, wrapping), RIGHT or a
// click opens a submenu, LEFT goes back up, a click on a leaf chooses it.
uint8_t menu_run(const struct menu *root,
                 const struct axis_calibration cal[ADC_NUM_CHANNELS]);

#endif
