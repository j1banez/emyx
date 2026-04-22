#include <stdint.h>

#include <kernel/arch.h>
#include <kernel/interrupt.h>
#include <kernel/panic.h>
#include <kernel/pmm.h>
#include <kernel/printk.h>
#include <kernel/serial.h>
#include <kernel/shell.h>
#include <kernel/timer.h>
#include <kernel/tty.h>
#include <kernel/vmm.h>

#define MULTIBOOT_MAGIC 0x2badb002

void kmain(uint32_t magic, uint32_t mbi_addr)
{
    serial_init();
    terminal_init();

    if (magic != MULTIBOOT_MAGIC) {
        panic("multiboot: wrong magic value\n");
    }

    pmm_init((void *)(uintptr_t)mbi_addr);
    vmm_init(VMM_BOOTSTRAP_LIMIT);
    arch_init();
    shell_init();
    irq_enable();

    while (1) {
        // Wait for interrupt
        arch_idle();
        timer_print();
    }
}
