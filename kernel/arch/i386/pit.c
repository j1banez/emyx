#include "io.h"
#include "pit.h"

#define PIT_COMMAND 0x43
#define PIT_CHANNEL0 0x40

#define PIT_MODE2 0x34

void pit_init(uint32_t hz)
{
    uint16_t divisor;

    if (hz == 0) {
        hz = 1;
    }

    divisor = (uint16_t)(1193182 / hz);

    if (divisor == 0) {
        divisor = 1;
    }

    outb(PIT_COMMAND, PIT_MODE2);
    outb(PIT_CHANNEL0, divisor & 0x00FF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0x00FF);
}
