#include <kernel/printk.h>
#include <kernel/serial.h>
#include <kernel/timer.h>

static volatile uint32_t ticks;
static volatile uint32_t last_printed_ticks;

void timer_tick(void)
{
    ticks++;
}

uint32_t timer_get_ticks(void)
{
    return ticks;
}

void timer_print(void)
{
    if (ticks - last_printed_ticks >= 100) {
        char ticks_hex[11];

        u32_to_hex(ticks_hex, ticks);
        serial_writestring("TICKS: ");
        serial_writestring(ticks_hex);
        serial_writestring("\n");

        last_printed_ticks = ticks;
    }
}
