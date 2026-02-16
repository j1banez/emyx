#include <kernel/interrupt.h>
#include <kernel/printk.h>
#include <kernel/serial.h>

static volatile uint32_t irq_counts[16];
static volatile uint32_t irq_last_prints[16];

void ex_handler(ex_report *report)
{
    char error_hex[11];
    char pc_hex[11];
    char status_hex[11];

    u32_to_hex(error_hex, report->error_code);
    u32_to_hex(pc_hex, report->pc); // TODO: uintptr_to_hex to support u64
    u32_to_hex(status_hex, report->status);

    printk("EXCEPTION: %s\n", report->name);
    printk("Error code: %s\n", error_hex);
    printk("PC: %s\n", pc_hex);
    printk("Status: %s\n", status_hex);

    serial_writestring("EXCEPTION: ");
    serial_writestring(report->name);
    serial_writestring("\n");
    serial_writestring("Error code: ");
    serial_writestring(error_hex);
    serial_writestring("\n");
    serial_writestring("PC: ");
    serial_writestring(pc_hex);
    serial_writestring("\n");
    serial_writestring("Status: ");
    serial_writestring(status_hex);
    serial_writestring("\n");

    // Print arch specific details
    for (int i = 0; i < report->extra_count; i++) {
        char extra_hex[11];

        u32_to_hex(extra_hex, report->extras[i].value);
        printk("%s: %s\n", report->extras[i].name, extra_hex);
        serial_writestring(report->extras[i].name);
        serial_writestring(": ");
        serial_writestring(extra_hex);
        serial_writestring("\n");
    }

    for (;;) { __asm__ volatile("cli; hlt"); }
}

void irq_handler(uint32_t irq)
{
    if (irq < 16)
        irq_counts[irq]++;

    irq_ack(irq);
}

void irq_print_stats(uint32_t interval)
{
    for (int i = 0; i < 16; i++) {
        // Rate limit, print every interval
        if (irq_counts[i] - irq_last_prints[i] >= interval) {
            char irq_hex[11];
            char count_hex[11];

            u32_to_hex(irq_hex, i);
            u32_to_hex(count_hex, irq_counts[i]);
            serial_writestring("IRQ ");
            serial_writestring(irq_hex);
            serial_writestring(" count=");
            serial_writestring(count_hex);
            serial_writestring("\n");

            irq_last_prints[i] = irq_counts[i];
        }
    }
}
