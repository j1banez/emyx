#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#include <stdint.h>

typedef struct {
    uint32_t vector;
    uint32_t error_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
} isr_frame;

void ex_handler(isr_frame *frame);
void irq_handler(uint32_t irq);
void irq_ack(uint32_t irq);

#endif
