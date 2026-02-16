#ifndef _IRQ_H
#define _IRQ_H

#include <stdint.h>

void irq_handler(uint32_t irq);
void irq_ack(uint32_t irq);

#endif
