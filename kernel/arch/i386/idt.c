#include <stdint.h>

#include "idt.h"

#define DECLARE_EX(n) extern void ex##n(void)
#define DECLARE_IRQ(n) extern void irq##n(void)

DECLARE_EX(0);
DECLARE_EX(1);
DECLARE_EX(2);
DECLARE_EX(3);
DECLARE_EX(4);
DECLARE_EX(5);
DECLARE_EX(6);
DECLARE_EX(7);
DECLARE_EX(8);
DECLARE_EX(9);
DECLARE_EX(10);
DECLARE_EX(11);
DECLARE_EX(12);
DECLARE_EX(13);
DECLARE_EX(14);
DECLARE_EX(15);
DECLARE_EX(16);
DECLARE_EX(17);
DECLARE_EX(18);
DECLARE_EX(19);
DECLARE_EX(20);
DECLARE_EX(21);
DECLARE_EX(22);
DECLARE_EX(23);
DECLARE_EX(24);
DECLARE_EX(25);
DECLARE_EX(26);
DECLARE_EX(27);
DECLARE_EX(28);
DECLARE_EX(29);
DECLARE_EX(30);
DECLARE_EX(31);

DECLARE_IRQ(0);
DECLARE_IRQ(1);
DECLARE_IRQ(2);
DECLARE_IRQ(3);
DECLARE_IRQ(4);
DECLARE_IRQ(5);
DECLARE_IRQ(6);
DECLARE_IRQ(7);
DECLARE_IRQ(8);
DECLARE_IRQ(9);
DECLARE_IRQ(10);
DECLARE_IRQ(11);
DECLARE_IRQ(12);
DECLARE_IRQ(13);
DECLARE_IRQ(14);
DECLARE_IRQ(15);

extern void isr_stub(void);

static idt_descriptor idt[256];
static idt_ptr idtr = {0, 0};

static void (*const exception_table[32])(void) = {
    ex0, ex1, ex2, ex3, ex4, ex5, ex6, ex7,
    ex8, ex9, ex10, ex11, ex12, ex13, ex14, ex15,
    ex16, ex17, ex18, ex19, ex20, ex21, ex22, ex23,
    ex24, ex25, ex26, ex27, ex28, ex29, ex30, ex31,
};

static void (*const irq_table[16])(void) = {
    irq0, irq1, irq2, irq3, irq4, irq5, irq6, irq7,
    irq8, irq9, irq10, irq11, irq12, irq13, irq14, irq15,
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

    // Register exceptions
    for (int i = 0; i < 32; i++) {
        idt[i] = new_descriptor((uint32_t)exception_table[i], 0x08, 0x8E);
    }

    // Register hardware interrupts
    for (int i = 0; i < 16; i++) {
        idt[i + 32] = new_descriptor((uint32_t)irq_table[i], 0x08, 0x8E);
    }

    idtr.base = (uint32_t)idt;
    idtr.limit = sizeof(idt) - 1;

    idt_flush(&idtr);
}
