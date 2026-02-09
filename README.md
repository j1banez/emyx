# Emyx

Emyx is an operating system with a kernel and libc written from scratch.

For now, it just can print text to screen using a basic VGA text-mode driver.

## Prerequisites

- Cross toolchain for `i686-elf` (`i686-elf-gcc`, `i686-elf-ar`, ...)
- `grub-mkrescue`
- `qemu-system-i386`

Toolchain setup notes: `docs/cross-compiler.md`

## Quick start

```sh
git clone https://github.com/j1banez/emyx.git
cd emyx
make run
```

Use `make help` for more information.

## Debug

```sh
make gdb

# In another terminal
gdb kernel/emyx.kernel
```

In GDB:
```sh
(gdb) target remote :1234
(gdb) b kmain
(gdb) c
(gdb) bt
(gdb) info registers
(gdb) n
[...]
```
