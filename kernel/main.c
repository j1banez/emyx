#include <stdint.h>

#include <kernel/arch.h>
#include <kernel/boot.h>
#include <kernel/interrupt.h>
#include <kernel/panic.h>
#include <kernel/pmm.h>
#include <kernel/printk.h>
#include <kernel/serial.h>
#include <kernel/sched.h>
#include <kernel/shell.h>
#include <kernel/timer.h>
#include <kernel/tty.h>
#include <kernel/vmm.h>

void kmain(uint32_t magic, uint32_t mbi_addr)
{
    multiboot_info *mbi;

    serial_init();
    terminal_init();

    if (magic != MULTIBOOT_MAGIC) {
        panic("multiboot: wrong magic value\n");
    }

    mbi = (multiboot_info *)(uintptr_t)mbi_addr;

    pmm_init(mbi);
    boot_init(mbi);
    vmm_init(VMM_BOOTSTRAP_LIMIT);
    arch_init();
    sched_init();
    shell_init();
    irq_enable();

    while (1) {
        // Wait for interrupt
        arch_idle();
        shell_poll();
        timer_print();
    }
}
