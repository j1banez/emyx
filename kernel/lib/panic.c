#include <kernel/arch.h>
#include <kernel/panic.h>
#include <kernel/printk.h>
#include <kernel/serial.h>

void panic(const char *err)
{
    serial_writestring(err);
    printk("%s", err);
    arch_halt();
}
