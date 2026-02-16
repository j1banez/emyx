#include <kernel/irq.h>

#include "pic.h"

void irq_ack(uint32_t irq)
{
    pic_send_eoi((uint8_t)irq);
}
