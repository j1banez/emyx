#include <stdint.h>

#include <kernel/keyboard.h>

#define KEYBOARD_BUFFER_SIZE 32u

/*
 * [a][b][c][ ][ ][ ][ ][ ]
 *  ^        ^
 * read     write
 */
static char buffer[KEYBOARD_BUFFER_SIZE];
static uint32_t read_index;
static uint32_t write_index;

void keyboard_buffer_push(char c)
{
    uint32_t next;

    if (c == '\0')
        return;

    next = (write_index + 1) % KEYBOARD_BUFFER_SIZE;
    if (next == read_index)
        return;

    buffer[write_index] = c;
    write_index = next;
}

uint32_t keyboard_buffer_pop(void)
{
    char c;

    if (read_index == write_index)
        return 0;

    c = buffer[read_index];
    read_index = (read_index + 1) % KEYBOARD_BUFFER_SIZE;

    return (uint32_t)c;
}

void keyboard_buffer_clear(void)
{
    read_index = write_index;
}
