#include <stdint.h>

#include <kernel/arch.h>
#include <kernel/boot.h>
#include <kernel/interrupt.h>
#include <kernel/panic.h>
#include <kernel/pmm.h>
#include <kernel/printk.h>
#include <kernel/serial.h>
#include <kernel/sched.h>
#include <kernel/kshell.h>
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

    mbi = vmm_phys_to_virt(mbi_addr);
    if (mbi == NULL)
        panic("multiboot: info is outside direct map");

    pmm_init(mbi);
    boot_init(mbi);
    vmm_init(VMM_DIRECT_MAP_LIMIT);
    arch_init();
    sched_init();
    kshell_init();
    irq_enable();

    while (1) {
        // Wait for interrupt
        arch_idle();
        kshell_poll();
        timer_print();
    }
}
