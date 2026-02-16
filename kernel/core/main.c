#include <kernel/arch.h>
#include <kernel/interrupt.h>
#include <kernel/printk.h>
#include <kernel/serial.h>
#include <kernel/tty.h>

static const uint32_t IRQ_PRINT_INTERVAL = 50;

void kmain(void) {
    arch_init();
    serial_init();

    serial_writestring("serial ok\n");
    terminal_initialize();
    printk("Hello, kernel World!\n");

    irq_enable();
    while (1) {
        // Wait for interrupt
        cpu_idle();
        irq_print_stats(IRQ_PRINT_INTERVAL);
    }
}
