#include <stddef.h>
#include <stdint.h>

#include <kernel/sched.h>

#define KTHREAD_STACK_ALIGN 16u

uintptr_t sched_prepare_kthread_stack(void *stack, size_t stack_size,
    void (*trampoline)(void))
{
    uintptr_t *stack_pointer;

    // i386 stacks grow downward; start from aligned high end.
    stack_pointer = (uintptr_t *)(((uintptr_t)stack + stack_size) &
        ~(uintptr_t)(KTHREAD_STACK_ALIGN - 1));

    // Fake caller return for trampoline if it ever returns.
    *--stack_pointer = 0;

    // Return target consumed by sched_context_switch's final ret.
    *--stack_pointer = (uintptr_t)trampoline;

    // Fake callee-saved register frame restored by context_switch.S.
    *--stack_pointer = 0;
    *--stack_pointer = 0;
    *--stack_pointer = 0;
    *--stack_pointer = 0;

    return (uintptr_t)stack_pointer;
}
