#include <kernel/irq.h>

void irq_handler(uint32_t irq)
{
    irq_ack(irq);
}
