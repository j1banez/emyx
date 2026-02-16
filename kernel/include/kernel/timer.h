#ifndef _TIMER_H
#define _TIMER_H

#include <stdint.h>

void timer_tick(void);
uint32_t timer_get_ticks(void);
void timer_print(void);

#endif
