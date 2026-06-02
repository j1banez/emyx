#ifndef _SCHED_H
#define _SCHED_H

#include <stddef.h>
#include <stdint.h>

void sched_init(void);
int kthread_create(void (*entry)(void));
void kthread_exit(void);
void sched_yield(void);
void sched_context_switch(uintptr_t *old_stack_pointer,
    uintptr_t new_stack_pointer);
uintptr_t sched_prepare_kthread_stack(void *stack, size_t stack_size,
    void (*trampoline)(void));

#endif
