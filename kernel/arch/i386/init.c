#include <kernel/arch.h>

#include "gdt.h"
#include "idt.h"
#include "io.h"
#include "pic.h"
#include "pit.h"

#define KBC_STATUS_PORT 0x64
#define KBC_RESET_CMD 0xFE

static void kbc_wait_write_ready(void)
{
    while (inb(KBC_STATUS_PORT) & 0x02)
        ;
}

void arch_init()
{
    gdt_init();
    idt_init();
    pic_remap(0x20, 0x28);
    pit_init(100);

    pic_clear_mask(0); // Timer
    pic_clear_mask(1); // Keyboard
    pic_clear_mask(2); // Slave PIC Cascade line
}

void arch_reboot(void)
{
    __asm__ volatile ("cli");

    kbc_wait_write_ready();
    outb(KBC_STATUS_PORT, KBC_RESET_CMD);

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
