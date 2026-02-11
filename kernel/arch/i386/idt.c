#include <stdint.h>

#include "idt.h"

#define DECLARE_ISR(n) extern void isr##n(void)

DECLARE_ISR(0);
DECLARE_ISR(1);
DECLARE_ISR(2);
DECLARE_ISR(3);
DECLARE_ISR(4);
DECLARE_ISR(5);
DECLARE_ISR(6);
DECLARE_ISR(7);
DECLARE_ISR(8);
DECLARE_ISR(9);
DECLARE_ISR(10);
DECLARE_ISR(11);
DECLARE_ISR(12);
DECLARE_ISR(13);
DECLARE_ISR(14);
DECLARE_ISR(15);
DECLARE_ISR(16);
DECLARE_ISR(17);
DECLARE_ISR(18);
DECLARE_ISR(19);
DECLARE_ISR(20);
DECLARE_ISR(21);
DECLARE_ISR(22);
DECLARE_ISR(23);
DECLARE_ISR(24);
DECLARE_ISR(25);
DECLARE_ISR(26);
DECLARE_ISR(27);
DECLARE_ISR(28);
DECLARE_ISR(29);
DECLARE_ISR(30);
DECLARE_ISR(31);

extern void isr_stub(void);

static idt_descriptor idt[256];
static idt_ptr idtr = {0, 0};

static void (*const isr_table[32])(void) = {
    isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7,
    isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15,
    isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
    isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31,
};

static idt_descriptor new_descriptor(
    uint32_t offset, uint16_t selector, uint8_t flags
)
{
    idt_descriptor descriptor;

    descriptor.offset_low = offset & 0x0000FFFF;
    descriptor.selector = selector;
    descriptor.zero = 0;
    descriptor.flags = flags;
    descriptor.offset_high = (offset & 0xFFFF0000) >> 16;

    return descriptor;
}

void idt_init()
{
    for (int i = 0; i < 256; i++) {
        idt[i] = new_descriptor((uint32_t)isr_stub, 0x08, 0x8E);
    }

    for (int i = 0; i < 32; i++) {
        idt[i] = new_descriptor((uint32_t)isr_table[i], 0x08, 0x8E);
    }

    idtr.base = (uint32_t)idt;
    idtr.limit = sizeof(idt) - 1;

    idt_flush(&idtr);
}
