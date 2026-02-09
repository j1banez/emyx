HOST ?= $(shell ./scripts/default-host.sh)
ARCH ?= $(shell ./scripts/target-triplet-to-arch.sh $(HOST))

PROJECTS := libc kernel
SYSTEM_HEADER_PROJECTS := libc kernel

AR := $(HOST)-ar
AS := $(HOST)-as
CC_BASE := $(HOST)-gcc

PREFIX ?= /usr
EXEC_PREFIX ?= $(PREFIX)
BOOTDIR ?= /boot
LIBDIR ?= $(EXEC_PREFIX)/lib
INCLUDEDIR ?= $(PREFIX)/include

CFLAGS ?= -O2 -g
CPPFLAGS ?=
export CFLAGS CPPFLAGS

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

iso: build
	mkdir -p isodir/boot/grub
	cp "$(SYSROOT)/boot/emyx.kernel" isodir/boot/emyx.kernel
	printf '%s\n' 'menuentry "emyx" {' '  multiboot /boot/emyx.kernel' '}' > isodir/boot/grub/grub.cfg
	grub-mkrescue -o emyx.iso isodir

run: iso
	qemu-system-$(ARCH) -cdrom emyx.iso -serial stdio

gdb: iso
	qemu-system-$(ARCH) -cdrom emyx.iso -serial stdio -s -S

clean:
	@for project in $(PROJECTS); do \
		$(MAKE) -C "$$project" clean; \
	done
	rm -rf "$(SYSROOT)" isodir emyx.iso
