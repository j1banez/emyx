.section .text

.global _start
.type _start, @function

.equ SYS_EXIT, 2
.equ USER_DATA_SELECTOR, 0x23

_start:
    mov $USER_DATA_SELECTOR, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    call main

    mov %eax, %ebx
    mov $SYS_EXIT, %eax
    xor %ecx, %ecx
    xor %edx, %edx
    int $0x80

1:
    jmp 1b
