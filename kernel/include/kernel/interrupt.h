#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#include <stdint.h>

typedef struct {
    const char *name;
    uintptr_t value;
} ex_extra;

typedef struct {
    const char *name;
    uint32_t vector;
    uint32_t error_code;
    uintptr_t pc;
    uintptr_t status;
    ex_extra extras[4];
    uint8_t extra_count;
} ex_report;

void ex_handler(ex_report *report);
void irq_handler(uint32_t irq);
void irq_ack(uint32_t irq);
void irq_enable(void);
void irq_disable(void);
void irq_print_stats(uint32_t interval);
const volatile uint32_t *irq_get_counts(void);

#endif
