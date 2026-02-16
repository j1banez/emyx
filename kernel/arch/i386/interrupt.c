#include <kernel/interrupt.h>

#include "ex.h"
#include "pic.h"

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

/*
 * Normalize i386 exception frame to kernel report
 */
void i386_ex_handler(ex_frame *frame)
{
    ex_extra cs = {
        .name = "CS",
        .value = frame->cs,
    };

    ex_report report = {
        .vector = frame->vector,
        .error_code = frame->error_code,
        .pc = frame->eip,
        .status = frame->eflags,
        .extras = {cs},
        .extra_count = 1,
    };

    if (frame->vector < 32) {
        report.name = exception_names[frame->vector];
    } else {
        report.name = "unknown";
    }

    ex_handler(&report);
}

void irq_ack(uint32_t irq)
{
    pic_send_eoi((uint8_t)irq);
}
