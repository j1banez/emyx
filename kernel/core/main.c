#include <kernel/printk.h>
#include <kernel/tty.h>

void kmain(void) {
    terminal_initialize();
	printk("Hello, kernel World!\n");
}
