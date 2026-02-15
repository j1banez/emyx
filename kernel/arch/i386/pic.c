#include "io.h"
#include "pic.h"

#define PIC1_COMMAND 0x20
#define PIC2_COMMAND 0xA0
#define PIC1_DATA 0x21
#define PIC2_DATA 0xA1

#define ICW1_INIT 0x10
#define ICW1_ICW4 0x01
#define ICW4_8086 0x01

void pic_remap(uint8_t master_offset, uint8_t slave_offset)
{
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    // ICW1
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    // ICW2, vector mapping
    outb(PIC1_DATA, master_offset); // IRQ0..7
    outb(PIC2_DATA, slave_offset); // IRQ8..15
    // ICW3
    outb(PIC1_DATA, 0x04); // Slave is on line 2
    outb(PIC2_DATA, 0x02); // Slave id is 2
    // ICW4
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);
    // Restore masks
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}
