#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include <kernel/printk.h>
#include <kernel/tty.h>

static char hex_digit(uint8_t nibble)
{
    if (nibble < 10) {
        return (char)('0' + nibble);
    }

    return (char)('a' + (nibble - 10));
}

void u32_to_hex(char out[11], uint32_t value)
{
    out[0] = '0';
    out[1] = 'x';

    for (int i = 0; i < 8; i++) {
        uint8_t nibble = (uint8_t)((value >> ((7 - i) * 4)) & 0x0f);
        out[2 + i] = hex_digit(nibble);
    }

    out[10] = '\0';
}

static bool print_bytes(const char* data, size_t length) {
    terminal_write(data, length);
    return true;
}

int printk(const char* format, ...) {
    va_list parameters;
    va_start(parameters, format);

    int written = 0;

    while (*format != '\0') {
        size_t maxrem = INT_MAX - written;

        if (format[0] != '%' || format[1] == '%') {
            if (format[0] == '%') {
                format++;
            }

            size_t amount = 1;
            while (format[amount] && format[amount] != '%') {
                amount++;
            }

            if (maxrem < amount) {
                return -1;
            }

            if (!print_bytes(format, amount)) {
                return -1;
            }

            format += amount;
            written += amount;
            continue;
        }

        const char* format_begun_at = format++;

        if (*format == 'c') {
            format++;
            char c = (char) va_arg(parameters, int);

            if (!maxrem) {
                return -1;
            }

            if (!print_bytes(&c, sizeof(c))) {
                return -1;
            }

            written++;
        } else if (*format == 's') {
            format++;
            const char* str = va_arg(parameters, const char*);

            if (!str) {
                str = "(null)";
            }

            size_t len = strlen(str);

            if (maxrem < len) {
                return -1;
            }

            if (!print_bytes(str, len)) {
                return -1;
            }

            written += len;
        } else if (*format == 'x') {
            char hex[11];

            format++;
            u32_to_hex(hex, va_arg(parameters, uint32_t));

            if (maxrem < 10) {
                return -1;
            }

            if (!print_bytes(hex, 10)) {
                return -1;
            }

            written += 10;
        } else {
            format = format_begun_at;
            size_t len = strlen(format);

            if (maxrem < len) {
                return -1;
            }

            if (!print_bytes(format, len)) {
                return -1;
            }

            written += len;
            format += len;
        }
    }

    va_end(parameters);
    return written;
}
