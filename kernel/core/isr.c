#include <kernel/isr.h>
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

void isr_handler(isr_frame *frame)
{
    const char *name;
    char error_hex[11];

    if (frame->vector < 32) {
        name = exception_names[frame->vector];
    } else {
        name = "unknown";
    }

    u32_to_hex(error_hex, frame->error_code);

    printk("EXCEPTION: %s\n", name);
    printk("Error code: %s\n", error_hex);
    serial_writestring("EXCEPTION: ");
    serial_writestring(name);
    serial_writestring("\n");
    serial_writestring("Error code: ");
    serial_writestring(error_hex);
    serial_writestring("\n");

    for (;;) { __asm__ volatile("cli; hlt"); }
}
