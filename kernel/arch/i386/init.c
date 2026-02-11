#include <kernel/arch.h>

#include "gdt.h"
#include "idt.h"

void arch_init()
{
    gdt_init();
    idt_init();
}
