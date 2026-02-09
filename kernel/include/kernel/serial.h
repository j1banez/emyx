#ifndef _KERNEL_SERIAL_H
#define _KERNEL_SERIAL_H

void serial_init(void);
void serial_write(char c);
void serial_writestring(const char* s);

#endif
