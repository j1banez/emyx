#include <kernel/interrupt.h>
#include <kernel/paging.h>

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
    ex_report report = {
        .vector = frame->vector,
        .error_code = frame->error_code,
        .pc = frame->eip,
        .status = frame->eflags,
        .extra_count = 0,
    };

    report.extras[report.extra_count++] = (ex_extra){
        .name = "CS",
        .value = frame->cs,
    };

    if (frame->vector == 14) {
        uint32_t error = frame->error_code;

        report.extras[report.extra_count++] = (ex_extra){
            .name = "CR2",
            .value = paging_fault_addr(),
        };
        report.extras[report.extra_count++] = (ex_extra){
            .name = "PF_PRESENT",
            .value = (error >> 0) & 0x1,
        };
        report.extras[report.extra_count++] = (ex_extra){
            .name = "PF_WRITE",
            .value = (error >> 1) & 0x1,
        };
        report.extras[report.extra_count++] = (ex_extra){
            .name = "PF_USER",
            .value = (error >> 2) & 0x1,
        };
        report.extras[report.extra_count++] = (ex_extra){
            .name = "PF_RSVD",
            .value = (error >> 3) & 0x1,
        };
        report.extras[report.extra_count++] = (ex_extra){
            .name = "PF_ID",
            .value = (error >> 4) & 0x1,
        };
    }

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

void irq_enable(void)
{
    __asm__ volatile("sti");
}

void irq_disable(void)
{
    __asm__ volatile("cli");
}
