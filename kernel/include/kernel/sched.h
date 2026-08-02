#ifndef _SCHED_H
#define _SCHED_H

#include <stddef.h>
#include <stdint.h>

void sched_init(void);
uint32_t sched_current_task_id(void);
int sched_set_current_address_space(uintptr_t address_space);
int kthread_create(void (*entry)(void));
void kthread_exit(void);
void sched_yield(void);
void sched_context_switch(uintptr_t *old_stack_pointer,
    uintptr_t new_stack_pointer);
void sched_set_kernel_task(void *stack, size_t stack_size);
uintptr_t sched_prepare_kthread_stack(void *stack, size_t stack_size,
    void (*trampoline)(void));

#endif
