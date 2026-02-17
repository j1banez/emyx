#include <kernel/arch.h>
#include <kernel/interrupt.h>
#include <kernel/serial.h>
#include <kernel/shell.h>
#include <kernel/timer.h>
#include <kernel/tty.h>

void kmain(void) {
    arch_init();
    serial_init();

    serial_writestring("serial ok\n");

    terminal_init();
    shell_init();

    irq_enable();
    while (1) {
        // Wait for interrupt
        cpu_idle();
        timer_print();
    }
}
