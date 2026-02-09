#include <kernel/printk.h>
#include <kernel/tty.h>
#include <kernel/serial.h>

void kmain(void) {
    serial_init();
    serial_writestring("serial ok\n");
    terminal_initialize();
	printk("Hello, kernel World!\n");
}
