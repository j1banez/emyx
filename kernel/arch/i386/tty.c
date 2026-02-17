#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <kernel/tty.h>

#include "io.h"
#include "vga.h"

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t* const VGA_MEMORY = (uint16_t*) 0xB8000;

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer;

static void terminal_update_cursor(void)
{
    uint16_t pos = (uint16_t)(terminal_row * VGA_WIDTH + terminal_column);
    outb(VGA_CRTC_INDEX, 0x0F);
    outb(VGA_CRTC_DATA, (uint8_t)(pos & 0xFF));
    outb(VGA_CRTC_INDEX, 0x0E);
    outb(VGA_CRTC_DATA, (uint8_t)((pos >> 8) & 0xFF));
}

void terminal_putentryat(unsigned char c, uint8_t color, size_t x, size_t y)
{
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

void terminal_init(void)
{
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_buffer = VGA_MEMORY;

    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            terminal_putentryat(' ', terminal_color, x, y);
        }
    }

    terminal_update_cursor();
}

void terminal_setcolor(uint8_t color)
{
    terminal_color = color;
}

void terminal_putchar(char c)
{
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT)
            terminal_scroll_up();
        terminal_update_cursor();
        return;
    }

    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);

    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT)
            terminal_scroll_up();
    }

    terminal_update_cursor();
}

void terminal_write(const char* data, size_t size)
{
    for (size_t i = 0; i < size; i++)
        terminal_putchar(data[i]);
}

void terminal_writestring(const char* data)
{
    terminal_write(data, strlen(data));
}

void terminal_scroll_up()
{
    size_t size = VGA_WIDTH * VGA_HEIGHT;

    for (size_t i = 0; i < size - VGA_WIDTH; i++)
        terminal_buffer[i] = terminal_buffer[i + VGA_WIDTH];

    for (size_t i = size - VGA_WIDTH; i < size; i++)
        terminal_buffer[i] = vga_entry(' ', terminal_color);

    terminal_row = VGA_HEIGHT - 1;
    terminal_column = 0;

    terminal_update_cursor();
}

void terminal_backspace(void)
{
    if (terminal_column == 0) {
        if (terminal_row == 0)
            return;

        terminal_column = VGA_WIDTH - 1;
        terminal_row--;
    } else {
        terminal_column--;
    }

    terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
    terminal_update_cursor();
}
