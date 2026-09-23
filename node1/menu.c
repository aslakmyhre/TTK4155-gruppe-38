#include "menu.h"
#include "io_board.h"
#include "joystick.h"
#include "oled.h"
#include "spi.h"

#include <avr/pgmspace.h>
#include <stdbool.h>
#include <stdio.h>
#include <util/delay.h>

#define MENU_MAX_DEPTH       4
#define MENU_POLL_MS         20
#define MENU_TITLE_LINE      0
#define MENU_FIRST_ITEM_LINE 1
#define MENU_VISIBLE_ITEMS   (OLED_LINES - MENU_FIRST_ITEM_LINE)

enum menu_event {
    EVENT_NONE,
    EVENT_UP,
    EVENT_DOWN,
    EVENT_BACK,
    EVENT_OPEN,
    EVENT_CLICK,
};

struct menu_input {
    enum joystick_direction direction;
    bool pressed;
};

struct menu_level {
    const struct menu *menu;
    uint8_t selected;
};


static struct menu_input read_input(const struct axis_calibration cal[ADC_NUM_CHANNELS]) {
    uint8_t raw[ADC_NUM_CHANNELS];
    adc_read(raw);
    struct joystick_position pos = joystick_position(
        calibration_apply(&cal[JOYSTICK_X_CHANNEL], raw[JOYSTICK_X_CHANNEL]),
        calibration_apply(&cal[JOYSTICK_Y_CHANNEL], raw[JOYSTICK_Y_CHANNEL]));
    // The IO-board button byte is not verified yet; any non-zero value counts as pressed
    struct menu_input input = {
        .direction = joystick_direction(pos),
        .pressed = io_board_joystick().button != 0,
    };
    return input;
}

// Only changes count, so holding the stick or the button gives a single event
static enum menu_event to_event(struct menu_input now, struct menu_input before) {
    if (now.pressed && !before.pressed) {
        return EVENT_CLICK;
    }
    if (now.direction == before.direction) {
        return EVENT_NONE;
    }
    switch (now.direction) {
        case JOYSTICK_UP:      return EVENT_UP;
        case JOYSTICK_DOWN:    return EVENT_DOWN;
        case JOYSTICK_LEFT:    return EVENT_BACK;
        case JOYSTICK_RIGHT:   return EVENT_OPEN;
        case JOYSTICK_NEUTRAL: return EVENT_NONE;
    }
    return EVENT_NONE;
}

// Scrolls only once the selection passes the bottom line
static uint8_t first_visible(uint8_t selected) {
    return selected < MENU_VISIBLE_ITEMS ? 0 : selected - MENU_VISIBLE_ITEMS + 1;
}

static void draw_item(const struct menu *menu, uint8_t index, uint8_t top, bool selected) {
    oled_clear_line(MENU_FIRST_ITEM_LINE + index - top);
    oled_print(selected ? "> " : "  ");
    oled_print(menu->items[index].label);
}

static void draw_menu(const struct menu *menu, uint8_t selected) {
    uint8_t top = first_visible(selected);
    oled_clear_line(MENU_TITLE_LINE);
    oled_print(menu->title);
    for (uint8_t line = 0; line < MENU_VISIBLE_ITEMS; line++) {
        uint8_t index = top + line;
        if (index < menu->count) {
            draw_item(menu, index, top, index == selected);
        } else {
            oled_clear_line(MENU_FIRST_ITEM_LINE + line);
        }
    }
}

// Redraws just the two affected lines unless the list has to scroll, since
// a full redraw over the slow SPI bus is visible.
static void move_selection(struct menu_level *level, uint8_t next) {
    uint8_t previous = level->selected;
    uint8_t top = first_visible(next);
    level->selected = next;
    if (top != first_visible(previous)) {
        draw_menu(level->menu, next);
        return;
    }
    draw_item(level->menu, previous, top, false);
    draw_item(level->menu, next, top, true);
}

uint8_t menu_run(const struct menu *root,
                 const struct axis_calibration cal[ADC_NUM_CHANNELS]) {
    struct menu_level path[MENU_MAX_DEPTH] = {{ .menu = root, .selected = 0 }};
    uint8_t depth = 0;
    draw_menu(root, 0);
    struct menu_input before = read_input(cal);

    while (1) {
        _delay_ms(MENU_POLL_MS);
        struct menu_input now = read_input(cal);
        if (spi_failed()) {
            return MENU_ACTION_SPI_FAILED;
        }
        enum menu_event event = to_event(now, before);
        before = now;

        struct menu_level *level = &path[depth];
        uint8_t count = level->menu->count;
        const struct menu_item *item = &level->menu->items[level->selected];

        switch (event) {
            case EVENT_UP:
                move_selection(level, level->selected == 0 ? count - 1 : level->selected - 1);
                break;
            case EVENT_DOWN:
                move_selection(level, (level->selected + 1) % count);
                break;
            case EVENT_BACK:
                if (depth > 0) {
                    depth--;
                    draw_menu(path[depth].menu, path[depth].selected);
                }
                break;
            case EVENT_OPEN:
            case EVENT_CLICK:
                if (item->submenu == NULL) {
                    if (event == EVENT_CLICK) {
                        return item->action;
                    }
                } else if (depth + 1 < MENU_MAX_DEPTH) {
                    depth++;
                    path[depth].menu = item->submenu;
                    path[depth].selected = 0;
                    draw_menu(item->submenu, 0);
                } else {
                    printf_P(PSTR("menu: '%s' nests deeper than MENU_MAX_DEPTH (%u)\n"),
                             item->label, MENU_MAX_DEPTH);
                }
                break;
            case EVENT_NONE:
                break;
        }
    }
}
