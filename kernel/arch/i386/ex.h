#ifndef _EX_H
#define _EX_H

#include <stdint.h>

typedef struct {
    uint32_t vector;
    uint32_t error_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
} ex_frame;

#endif
