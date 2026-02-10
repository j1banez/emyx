.section .text
.global gdt_flush
.type gdt_flush, @function

gdt_flush:
  # Get gdt_ptr* argument from stack into eax
  movl 4(%esp), %eax
  lgdt (%eax)

  # Reload data segment registers
  movw $0x10, %ax
  movw %ax, %ds
  movw %ax, %es
  movw %ax, %fs
  movw %ax, %gs
  movw %ax, %ss

  # Jump to reload CS with 0x08 (code segment)
  ljmp $0x08, $gdt_flush_done

gdt_flush_done:
  ret
