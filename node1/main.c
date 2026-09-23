#include "oled.h"

int main(void)
{
    oled_init();

    oled_goto(0, 0);
    oled_putchar('A');           /* a single character, top left */

    oled_goto(3, 40);
    oled_print("HELLO 123");     /* a string in the middle */

    while (1) { }
}