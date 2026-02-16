.section .text

.extern irq_handler

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
