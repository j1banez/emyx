#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include <stdint.h>

uint8_t keyboard_read(void);
char keyboard_decode(uint8_t scancode);
void keyboard_buffer_push(char c);
uint32_t keyboard_buffer_pop(void);
void keyboard_buffer_clear(void);

#endif
