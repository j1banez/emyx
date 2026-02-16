#include <kernel/keyboard.h>

#include "io.h"

#define KEYBOARD_DATA 0x60

uint8_t keyboard_read(void)
{
    return inb(KEYBOARD_DATA);
}
