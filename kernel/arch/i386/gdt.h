#ifndef _GDT_H
#define _GDT_H

#include <stdint.h>

#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10
#define GDT_USER_CODE 0x18
#define GDT_USER_DATA 0x20
#define GDT_TSS 0x28

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed)) gdt_descriptor;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdt_ptr;

void gdt_init(void);
void gdt_flush(gdt_ptr *gdtr);
void tss_flush(uint16_t selector);
void tss_set_kernel_stack(uintptr_t stack);

#endif
