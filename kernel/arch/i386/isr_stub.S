.section .text

.extern i386_ex_handler
.extern irq_handler

.macro EX_ERR n
.global ex\n
.type ex\n, @function
ex\n:
    push $\n
    push %esp
    call i386_ex_handler
    add $4, %esp
    add $8, %esp
    cli
1:
    hlt
    jmp 1b
.endm

.macro EX_NOERR n
.global ex\n
.type ex\n, @function
ex\n:
    push $0 # Synthetic error code
    push $\n
    push %esp
    call i386_ex_handler
    add $4, %esp
    add $8, %esp
    cli
1:
    hlt
    jmp 1b
.endm

.macro IRQ n
.global irq\n
.type irq\n, @function
irq\n:
    pusha
    push $\n
    call irq_handler
    add $4, %esp
    popa
    iret
.endm

.global isr_stub
.type isr_stub, @function
isr_stub:
    cli
1:
    hlt
    jmp 1b

EX_NOERR 0
EX_NOERR 1
EX_NOERR 2
EX_NOERR 3
EX_NOERR 4
EX_NOERR 5
EX_NOERR 6
EX_NOERR 7
EX_ERR 8
EX_NOERR 9
EX_ERR 10
EX_ERR 11
EX_ERR 12
EX_ERR 13
EX_ERR 14
EX_NOERR 15
EX_NOERR 16
EX_ERR 17
EX_NOERR 18
EX_NOERR 19
EX_NOERR 20
EX_NOERR 21
EX_NOERR 22
EX_NOERR 23
EX_NOERR 24
EX_NOERR 25
EX_NOERR 26
EX_NOERR 27
EX_NOERR 28
EX_NOERR 29
EX_NOERR 30
EX_NOERR 31

IRQ 0
IRQ 1
IRQ 2
IRQ 3
IRQ 4
IRQ 5
IRQ 6
IRQ 7
IRQ 8
IRQ 9
IRQ 10
IRQ 11
IRQ 12
IRQ 13
IRQ 14
IRQ 15
