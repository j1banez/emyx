#include <kernel/serial.h>
#include "io.h"

#define COM1 0x3F8

// Quick cheat sheet:
// +0
//  - DLAB=0: data register (TX write / RX read)
//  - DLAB=1: divisor low byte
// +1
//  - DLAB=0: interrupt enable (IER)
//  - DLAB=1: divisor high byte
// +2
//  - write: FIFO control (FCR)
//  - read: interrupt ID (IIR)
// +3
//  - line control (LCR), includes DLAB bit (bit 7)
// +4
//  - modem control (MCR)
// +5
//  - line status (LSR)
//  - bit 0x20 = THRE (1 -> ready to send)
// +6
//  - modem status (MSR)
// +7
//  - scratch register

static int is_serial_transmit_empty(void) {
    // Check THRE bit
    return inb(COM1 + 5) & 0x20;
}

void serial_init(void) {
    outb(COM1 + 1, 0x00); // Disable interrupts
    outb(COM1 + 3, 0x80); // Enable DLAB
    outb(COM1 + 0, 0x03); // Divisor low (38400 baud)
    outb(COM1 + 1, 0x00); // Divisor high
    outb(COM1 + 3, 0x03); // 8 bits, no parity, 1 stop bit (8N1)
    outb(COM1 + 2, 0xC7); // Enable FIFO, clear, 14-byte threshold
    outb(COM1 + 4, 0x0B); // IRQs off for now, RTS/DTR set
}

void serial_write(char c) {
    while (!is_serial_transmit_empty()) { }
    outb(COM1, (unsigned char)c);
}

void serial_writestring(const char* s) {
    while (*s) {
        // Converts LF to CR+LF behavior for terminal compatibility.
        if (*s == '\n') serial_write('\r');
        serial_write(*s++);
    }
}
