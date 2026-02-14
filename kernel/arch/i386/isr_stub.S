.section .text

.extern isr_handler

.macro ISR_ERR n
.global isr\n
.type isr\n, @function
isr\n:
    push $\n
    push %esp
    call isr_handler
    add $4, %esp
    add $8, %esp
    cli
1:
    hlt
    jmp 1b
.endm

.macro ISR_NOERR n
.global isr\n
.type isr\n, @function
isr\n:
    push $0 # Synthetic error code
    push $\n
    push %esp
    call isr_handler
    add $4, %esp
    add $8, %esp
    cli
1:
    hlt
    jmp 1b
.endm


.global isr_stub
.type isr_stub, @function
isr_stub:
    cli
1:
    hlt
    jmp 1b

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR 8
ISR_NOERR 9
ISR_ERR 10
ISR_ERR 11
ISR_ERR 12
ISR_ERR 13
ISR_ERR 14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31
