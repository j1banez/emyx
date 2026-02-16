#include <kernel/arch.h>
#include <kernel/interrupt.h>
#include <kernel/printk.h>
#include <kernel/serial.h>
#include <kernel/timer.h>
#include <kernel/tty.h>

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
        timer_print();
    }
}
