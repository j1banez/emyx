#include <kernel/arch.h>

#include "gdt.h"
#include "idt.h"
#include "pic.h"

void arch_init()
{
    gdt_init();
    idt_init();
    pic_remap(0x20, 0x28);
    pic_clear_mask(0); // Timer
    pic_clear_mask(2); // Slave PIC Cascade line
}
