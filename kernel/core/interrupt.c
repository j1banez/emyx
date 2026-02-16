#include <kernel/interrupt.h>
#include <kernel/printk.h>
#include <kernel/serial.h>

static const char *const exception_names[32] = {
    "#DE divide by zero",
    "#DB debug",
    "NMI interrupt",
    "#BP breakpoint",
    "#OF overflow",
    "#BR bound range exceeded",
    "#UD invalid opcode",
    "#NM device not available",
    "#DF double fault",
    "coprocessor segment overrun",
    "#TS invalid TSS",
    "#NP segment not present",
    "#SS stack-segment fault",
    "#GP general protection fault",
    "#PF page fault",
    "reserved",
    "#MF x87 floating-point exception",
    "#AC alignment check",
    "#MC machine check",
    "#XM SIMD floating-point exception",
    "#VE virtualization exception",
    "#CP control protection exception",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "#HV hypervisor injection exception",
    "#VC VMM communication exception",
    "#SX security exception",
    "reserved",
};

void ex_handler(isr_frame *frame)
{
    const char *name;
    char error_hex[11];
    char eip_hex[11];
    char cs_hex[11];
    char eflags_hex[11];

    if (frame->vector < 32) {
        name = exception_names[frame->vector];
    } else {
        name = "unknown";
    }

    u32_to_hex(error_hex, frame->error_code);
    u32_to_hex(eip_hex, frame->eip);
    u32_to_hex(cs_hex, frame->cs);
    u32_to_hex(eflags_hex, frame->eflags);

    printk("EXCEPTION: %s\n", name);
    printk("Error code: %s\n", error_hex);
    printk("EIP: %s\n", eip_hex);
    printk("CS: %s\n", cs_hex);
    printk("EFLAGS: %s\n", eflags_hex);
    serial_writestring("EXCEPTION: ");
    serial_writestring(name);
    serial_writestring("\n");
    serial_writestring("Error code: ");
    serial_writestring(error_hex);
    serial_writestring("\n");
    serial_writestring("EIP: ");
    serial_writestring(eip_hex);
    serial_writestring("\n");
    serial_writestring("CS: ");
    serial_writestring(cs_hex);
    serial_writestring("\n");
    serial_writestring("EFLAGS: ");
    serial_writestring(eflags_hex);
    serial_writestring("\n");

    for (;;) { __asm__ volatile("cli; hlt"); }
}

void irq_handler(uint32_t irq)
{
    irq_ack(irq);
}
