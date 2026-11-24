# Copyright (C) 2026 Ivan Gaydardzhiev
# Licensed under the GPL-3.0-only
#
# Native build on aarch64 (Termux proot Void: just `make`). Cross build:
#   make CC="clang --target=aarch64-linux-gnu" LDFLAGS_EXTRA=-fuse-ld=lld

ifeq ($(origin CC),default)
CC=gcc
endif
AR ?= ar

PREFIX ?= /opt/a64libc
BINDIR ?= /usr/local/bin
DESTDIR ?=
LIBDIR=$(PREFIX)/lib
INCDIR=$(PREFIX)/include
SPECS=$(LIBDIR)/a64libc-gcc.specs

# gcc only: stop the compiler turning the byte loops in memset/memcpy into calls to themselves
LOOPFLAG := $(shell $(CC) -fno-tree-loop-distribute-patterns -Werror -x c -c /dev/null -o /dev/null 2>/dev/null && echo -fno-tree-loop-distribute-patterns)
# the compiler's own freestanding headers (stdarg.h, stdbool.h, limits.h), found last
FREEINC := $(shell d=$$($(CC) -print-file-name=include 2>/dev/null); [ -d "$$d" ] || d=$$($(CC) -print-resource-dir 2>/dev/null)/include; [ -d "$$d" ] && echo "$$d")

CFLAGS=-march=armv8-a -mgeneral-regs-only -ffreestanding -fno-builtin -nostdinc \
	-fno-stack-protector -fno-asynchronous-unwind-tables -fno-pie -Os -Wall -Wextra \
	$(LOOPFLAG) -I include $(if $(FREEINC),-isystem $(FREEINC))
LDFLAGS=-static -no-pie -Wno-unused-command-line-argument -nostdlib -Wl,-z,max-page-size=0x10000 -T linker.ld $(LDFLAGS_EXTRA)

CRT0=crt0.o
LIBOBJS=syscalls.o malloc.o stdio.o
ARCHIVE=a64libc.a
TESTS=hello fmt bf

all: $(ARCHIVE) $(CRT0)

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(ARCHIVE): $(LIBOBJS)
	rm -f $@
	$(AR) rcs $@ $(LIBOBJS)

tests: $(TESTS)

$(TESTS): %: test/%.c $(CRT0) $(ARCHIVE)
	$(CC) $(CFLAGS) test/$*.c $(CRT0) $(ARCHIVE) $(LDFLAGS) -o $@

check: hello fmt
	@out=$$(./hello 2>&1); echo "$$out"; case "$$out" in *FAIL*) echo "hello: FAILED"; exit 1;; esac
	@./fmt a b > /dev/null && echo "fmt: ALL OK" || { echo "fmt: FAILED (run ./fmt)"; exit 1; }

run: hello
	@./hello; echo "exit code: $$?"

diag: hello
	ls -lh hello
	printf "\n\n"
	file hello
	printf "\n\n"
	size hello
	printf "\n\n"
	readelf -hl hello
	printf "\n\n"
	./hello; echo "exit code: $$?"

bf: test/bf.c $(CRT0) $(ARCHIVE)

runbf: bf
	@echo '++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++.' | ./bf

install: all
	mkdir -p $(DESTDIR)$(LIBDIR) $(DESTDIR)$(INCDIR) $(DESTDIR)$(BINDIR)
	cp $(ARCHIVE) $(CRT0) $(DESTDIR)$(LIBDIR)/
	cp linker.ld $(DESTDIR)$(LIBDIR)/a64libc.ld
	cp include/*.h $(DESTDIR)$(INCDIR)/
	sh gen_specs.sh $(LIBDIR) $(INCDIR) $(DESTDIR)$(SPECS) "$(FREEINC)"
	for w in a64libc-gcc a64libc-tcc; do \
		sed 's|/opt/a64libc|$(PREFIX)|g' $$w > $(DESTDIR)$(BINDIR)/$$w; \
		chmod +x $(DESTDIR)$(BINDIR)/$$w; \
	done
	@echo installed a64libc to $(DESTDIR)$(PREFIX)
	@echo wrappers: $(DESTDIR)$(BINDIR)/a64libc-gcc $(DESTDIR)$(BINDIR)/a64libc-tcc

uninstall:
	rm -rf $(DESTDIR)$(PREFIX)
	rm -f $(DESTDIR)$(BINDIR)/a64libc-gcc $(DESTDIR)$(BINDIR)/a64libc-tcc
	@echo uninstalled a64libc

clean:
	rm -f *.o $(TESTS) $(ARCHIVE)

.PHONY: all tests check run runbf diag install uninstall clean
