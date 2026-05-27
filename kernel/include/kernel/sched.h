#ifndef _SCHED_H
#define _SCHED_H

void sched_init(void);
int kthread_create(void (*entry)(void));
void sched_yield(void);

#endif
