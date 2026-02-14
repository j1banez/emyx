#ifndef _ISR_H
#define _ISR_H

#include <stdint.h>

typedef struct {
    uint32_t vector;
    uint32_t error_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
} isr_frame;

void isr_handler(isr_frame *frame);

#endif
