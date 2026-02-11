#ifndef _IDT_H
#define _IDT_H

#include <stdint.h>

typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero; // Unused, set to 0
    uint8_t flags;
    uint16_t offset_high;
} __attribute__((packed)) idt_descriptor;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr;

void idt_init(void);
void idt_flush(idt_ptr *idtr);

#endif
