.section .text

.equ SYS_WRITE, 1
.equ SYS_EXIT, 2
.equ SYS_GETC, 4
.equ SYS_FD_STDOUT, 1
.equ USER_DATA_SELECTOR, 0x23
.equ PROMPT_LEN, 11
.equ KEY_LEN, 2

.byte 'E', 'M', 'X', 'F'
.long readkey_code_end - readkey_code_start
.long 0

readkey_code_start:
    mov $USER_DATA_SELECTOR, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    pushl $0x000a3a79
    pushl $0x656b2073
    pushl $0x73657270

    mov $SYS_WRITE, %eax
    mov $SYS_FD_STDOUT, %ebx
    mov %esp, %ecx
    mov $PROMPT_LEN, %edx
    int $0x80
    add $12, %esp

1:
    mov $SYS_GETC, %eax
    int $0x80
    test %eax, %eax
    jnz 2f
    jmp 1b

2:
    sub $4, %esp
    mov %al, (%esp)
    movb $0x0a, 1(%esp)

    mov $SYS_WRITE, %eax
    mov $SYS_FD_STDOUT, %ebx
    mov %esp, %ecx
    mov $KEY_LEN, %edx
    int $0x80

    mov $SYS_EXIT, %eax
    xor %ebx, %ebx
    xor %ecx, %ecx
    xor %edx, %edx
    int $0x80

3:
    jmp 3b
readkey_code_end:
