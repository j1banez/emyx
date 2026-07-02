.section .text

.equ SYS_WRITE, 1
.equ SYS_EXIT, 2
.equ SYS_FD_STDOUT, 1
.equ USER_DATA_SELECTOR, 0x23
.equ HELLO_MESSAGE_LEN, 16

.byte 'E', 'M', 'X', 'F'
.long hello_code_end - hello_code_start
.long 0

hello_code_start:
    mov $USER_DATA_SELECTOR, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    pushl $0x0a6e6962
    pushl $0x2f206d6f
    pushl $0x7266206f
    pushl $0x6c6c6568

    mov $SYS_WRITE, %eax
    mov $SYS_FD_STDOUT, %ebx
    mov %esp, %ecx
    mov $HELLO_MESSAGE_LEN, %edx
    int $0x80

    mov $SYS_EXIT, %eax
    xor %ebx, %ebx
    xor %ecx, %ecx
    xor %edx, %edx
    int $0x80

1:
    jmp 1b
hello_code_end:
