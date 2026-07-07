HOST ?= $(shell ./scripts/default-host.sh)
ARCH ?= $(shell ./scripts/target-triplet-to-arch.sh $(HOST))

PROJECTS := libc kernel
SYSTEM_HEADER_PROJECTS := libc kernel

AR := $(HOST)-ar
AS := $(HOST)-as
OBJCOPY := $(HOST)-objcopy
CC_BASE := $(HOST)-gcc
QEMU := qemu-system-$(ARCH)

PREFIX ?= /usr
EXEC_PREFIX ?= $(PREFIX)
BOOTDIR ?= /boot
LIBDIR ?= $(EXEC_PREFIX)/lib
INCLUDEDIR ?= $(PREFIX)/include

CFLAGS ?= -O2 -g
CPPFLAGS ?=
export CFLAGS CPPFLAGS

QEMU_FLAGS ?= -cdrom emyx.iso -serial stdio -display sdl

SYSROOT := $(CURDIR)/sysroot
CC := $(CC_BASE) --sysroot=$(SYSROOT)

# This build stays freestanding, but the kernel intentionally depends on the
# project libc subset (libk) for shared low-level interfaces. That means kernel
# #include <...> must resolve to target headers installed in this project's
# sysroot (e.g. sysroot/usr/include), never host machine headers.
# Some *-elf cross compilers built with --without-headers do not provide a
# default target system include search path, so we force it with:
#   --sysroot=$(SYSROOT) + -isystem=$(INCLUDEDIR)
ifneq ($(filter %-elf %-elf-%,$(HOST)),)
CC := $(CC) -isystem=$(INCLUDEDIR)
endif

SUBMAKE_VARS := AR="$(AR)" CC="$(CC)" PREFIX="$(PREFIX)" EXEC_PREFIX="$(EXEC_PREFIX)" BOOTDIR="$(BOOTDIR)" LIBDIR="$(LIBDIR)" INCLUDEDIR="$(INCLUDEDIR)"

.PHONY: all help explain headers build iso run clean

all: iso

help:
	@printf '%s\n' \
		"Targets:" \
		"  explain  Show resolved build variables" \
		"  headers  Install system headers into sysroot" \
		"  build    Build and install libc + kernel into sysroot" \
		"  iso      Build bootable emyx.iso" \
		"  run      Build ISO and run in QEMU" \
		"  gdb      Build ISO and run in QEMU with GDB" \
		"  clean    Clean all build outputs"

explain:
	@printf '%s\n' \
		"Build configuration:" \
		"  HOST=$(HOST)" \
		"  ARCH=$(ARCH)" \
		"  SYSROOT=$(SYSROOT)" \
		"  AR=$(AR)" \
		"  AS=$(AS)" \
		"  CC_BASE=$(CC_BASE)" \
		"  CC=$(CC)" \
		"  PREFIX=$(PREFIX)" \
		"  EXEC_PREFIX=$(EXEC_PREFIX)" \
		"  BOOTDIR=$(BOOTDIR)" \
		"  LIBDIR=$(LIBDIR)" \
		"  INCLUDEDIR=$(INCLUDEDIR)" \
		"  ELF include workaround active=$(if $(filter %-elf %-elf-%,$(HOST)),yes,no)"

$(SYSROOT):
	mkdir -p "$(SYSROOT)"

headers: $(SYSROOT)
	@for project in $(SYSTEM_HEADER_PROJECTS); do \
		$(MAKE) -C "$$project" DESTDIR="$(SYSROOT)" $(SUBMAKE_VARS) install-headers; \
	done

build: headers
	@for project in $(PROJECTS); do \
		$(MAKE) -C "$$project" DESTDIR="$(SYSROOT)" $(SUBMAKE_VARS) install; \
	done

user/%.emxf: user/%.S
	$(CC_BASE) -MD -c $< -o user/$*.o -ffreestanding -Wall -Wextra
	$(OBJCOPY) -O binary -j .text user/$*.o $@

user/crt0.o: user/crt0.S
	$(CC_BASE) -MD -c $< -o $@ -ffreestanding -Wall -Wextra

user/%.o: user/%.c user/user.h
	$(CC_BASE) -MD -c $< -o $@ -std=gnu11 -ffreestanding -Wall -Wextra -fno-pic -fno-pie

user/%.elf: user/crt0.o user/%.o user/emxf.ld
	$(CC_BASE) -T user/emxf.ld -o $@ -ffreestanding -nostdlib user/crt0.o user/$*.o -lgcc

user/%.emxf: user/%.elf
	$(OBJCOPY) -O binary $< $@
	python3 scripts/check-emxf.py $@ $<

user/initramfs.emxa: user/init.emxf user/hello.emxf user/readkey.emxf scripts/mkemxa.py
	python3 scripts/mkemxa.py $@ /bin/init user/init.emxf /bin/hello user/hello.emxf /bin/readkey user/readkey.emxf

iso: build user/initramfs.emxa
	mkdir -p isodir/boot/grub
	cp "$(SYSROOT)/boot/emyx.kernel" isodir/boot/emyx.kernel
	cp user/initramfs.emxa isodir/boot/initramfs.emxa
	cp boot/grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o emyx.iso isodir

run: iso
	$(QEMU) $(QEMU_FLAGS)

gdb: iso
	$(QEMU) $(QEMU_FLAGS) -s -S

clean:
	@for project in $(PROJECTS); do \
		$(MAKE) -C "$$project" clean; \
	done
	rm -f user/*.o user/*.d user/*.emxf user/*.emxa
	rm -f user/*.bin user/*.elf
	rm -rf "$(SYSROOT)" isodir emyx.iso
