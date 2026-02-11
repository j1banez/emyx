.section .text
.global idt_flush
.type idt_flush, @function

idt_flush:
  # Get idt_ptr* argument from stack into eax
  movl 4(%esp), %eax
  lidt (%eax)
  ret
