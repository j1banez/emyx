#include <stdint.h>

#include "gdt.h"

#define GDT_ENTRIES 6u
#define TSS_ACCESS 0x89
#define TSS_FLAGS 0x00
#define SEGMENT_LIMIT_4G 0xFFFFF
#define SEGMENT_FLAGS_4K_32BIT 0xC0

typedef struct {
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iopb;
} __attribute__((packed)) i386_tss;

extern uint8_t stack_top[];

static gdt_descriptor gdt[GDT_ENTRIES];
static gdt_ptr gdtr = {0, 0};
static i386_tss tss;

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

static void tss_init(void)
{
    // TODO: update esp0 per user task before returning to ring 3.
    tss.ss0 = GDT_KERNEL_DATA;
    tss.esp0 = (uint32_t)stack_top;
    tss.iopb = sizeof(tss);
}

/*
 * Flat segmentation model:
 * - code segment: base 0, full 4 GiB span, read/execute
 * - data segment: base 0, full 4 GiB span, read/write
 */
void gdt_init()
{
    tss_init();

    gdt[0] = new_descriptor(0, 0, 0, 0); // Null descriptor
    gdt[1] = new_descriptor(0, SEGMENT_LIMIT_4G, 0x9A,
        SEGMENT_FLAGS_4K_32BIT); // Kernel code
    gdt[2] = new_descriptor(0, SEGMENT_LIMIT_4G, 0x92,
        SEGMENT_FLAGS_4K_32BIT); // Kernel data
    gdt[3] = new_descriptor(0, SEGMENT_LIMIT_4G, 0xFA,
        SEGMENT_FLAGS_4K_32BIT); // User code
    gdt[4] = new_descriptor(0, SEGMENT_LIMIT_4G, 0xF2,
        SEGMENT_FLAGS_4K_32BIT); // User data
    gdt[5] = new_descriptor((uint32_t)&tss, sizeof(tss) - 1, TSS_ACCESS,
        TSS_FLAGS); // TSS

    gdtr.base = (uint32_t)gdt;
    gdtr.limit = sizeof(gdt) - 1;

    gdt_flush(&gdtr);
    tss_flush(GDT_TSS);
}
