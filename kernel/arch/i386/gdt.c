#include <stdint.h>

#include "gdt.h"

static gdt_descriptor gdt[3];
static gdt_ptr gdtr = {0, 0};

static gdt_descriptor new_descriptor(
    uint32_t base, uint32_t limit, uint8_t access, uint8_t flags
)
{
    gdt_descriptor descriptor;

    descriptor.limit_low = limit & 0x0000FFFF;
    descriptor.base_low = base & 0x0000FFFF;
    descriptor.base_middle = (base & 0x00FF0000) >> 16;
    descriptor.access = access;
    descriptor.granularity = (flags & 0xF0) | ((limit >> 16) & 0x0F);
    descriptor.base_high = (base & 0xFF000000) >> 24;

    return descriptor;
}

/*
 * Flat segmentation model:
 * - code segment: base 0, full 4 GiB span, read/execute
 * - data segment: base 0, full 4 GiB span, read/write
 */
void gdt_init()
{
    gdt[0] = new_descriptor(0, 0, 0, 0); // Null descriptor
    gdt[1] = new_descriptor(0, 0xFFFFF, 0x9A, 0xC0); // Kernel code
    gdt[2] = new_descriptor(0, 0xFFFFF, 0x92, 0xC0); // Kernel data

    gdtr.base = (uint32_t)gdt;
    gdtr.limit = sizeof(gdt) - 1;

    gdt_flush(&gdtr);
}
