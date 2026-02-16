#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include <stdint.h>

uint8_t keyboard_read(void);
char keyboard_decode(uint8_t scancode);

#endif
