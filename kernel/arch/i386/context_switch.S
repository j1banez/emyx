.section .text
.global sched_context_switch
.type sched_context_switch, @function

sched_context_switch:
    # Save callee-saved registers on the old task stack.
    push %ebp
    push %ebx
    push %esi
    push %edi

    # Store old esp, then move to the new task stack.
    mov 20(%esp), %eax
    mov 24(%esp), %edx
    mov %esp, (%eax)
    mov %edx, %esp

    # Restore callee-saved registers from the new task stack.
    pop %edi
    pop %esi
    pop %ebx
    pop %ebp

    # Resume existing task, or enter a new task trampoline.
    ret
