HOST?=$(shell ../scripts/default-host.sh)
HOSTARCH=$(shell ../scripts/target-triplet-to-arch.sh $(HOST))

CFLAGS?=-O2 -g
CPPFLAGS?=
LDFLAGS?=
LIBS?=

DESTDIR?=
PREFIX?=/usr/local
EXEC_PREFIX?=$(PREFIX)
INCLUDEDIR?=$(PREFIX)/include
BOOTDIR?=$(EXEC_PREFIX)/boot

CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra
CPPFLAGS:=$(CPPFLAGS) -D__is_kernel -Iinclude
LDFLAGS:=$(LDFLAGS)
LIBS:=$(LIBS) -nostdlib -lk -lgcc

ARCHDIR=arch/$(HOSTARCH)

include $(ARCHDIR)/make.config

CFLAGS:=$(CFLAGS) $(KERNEL_ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)
LDFLAGS:=$(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS:=$(LIBS) $(KERNEL_ARCH_LIBS)

KERNEL_OBJS=\
$(KERNEL_ARCH_OBJS) \
lib/printk.o \
core/main.o \
core/interrupt.o \
core/timer.o \

OBJS=\
$(ARCHDIR)/crti.o \
$(ARCHDIR)/crtbegin.o \
$(KERNEL_OBJS) \
$(ARCHDIR)/crtend.o \
$(ARCHDIR)/crtn.o \

LINK_LIST=\
$(LDFLAGS) \
$(ARCHDIR)/crti.o \
$(ARCHDIR)/crtbegin.o \
$(KERNEL_OBJS) \
$(LIBS) \
$(ARCHDIR)/crtend.o \
$(ARCHDIR)/crtn.o \

.PHONY: all clean install install-headers install-kernel explain

all: emyx.kernel

explain:
	@printf '%s\n' \
		"Kernel build configuration:" \
		"  HOST=$(HOST)" \
		"  HOSTARCH=$(HOSTARCH)" \
		"  ARCHDIR=$(ARCHDIR)" \
		"  DESTDIR=$(DESTDIR)" \
		"  PREFIX=$(PREFIX)" \
		"  EXEC_PREFIX=$(EXEC_PREFIX)" \
		"  BOOTDIR=$(BOOTDIR)" \
		"  INCLUDEDIR=$(INCLUDEDIR)" \
		"  CC=$(CC)" \
		"  CFLAGS=$(CFLAGS)" \
		"  CPPFLAGS=$(CPPFLAGS)" \
		"  LDFLAGS=$(LDFLAGS)" \
		"  LIBS=$(LIBS)"

emyx.kernel: $(OBJS) $(ARCHDIR)/linker.ld
	$(CC) -T $(ARCHDIR)/linker.ld -o $@ $(CFLAGS) $(LINK_LIST)
	grub-file --is-x86-multiboot $@

$(ARCHDIR)/crtbegin.o $(ARCHDIR)/crtend.o:
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@

%.o: %.c
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

%.o: %.S
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

clean:
	rm -f emyx.kernel
	rm -f $(OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d

install: install-headers install-kernel

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

install-kernel: emyx.kernel
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp emyx.kernel $(DESTDIR)$(BOOTDIR)

-include $(OBJS:.o=.d)
