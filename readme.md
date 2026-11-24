a64libc
=======

## NAME

a64libc - C runtime for 64-bit ARM (AArch64) Linux, built for GrapheneOS and Termux proot Void Linux (musl)

## SYNOPSIS

```
a64libc-gcc [-c] [-o out] [-I dir] [-D name[=value]] [-On] file ...
a64libc-tcc file.c [-o out]
make [all | tests | check | run | runbf | diag | install | uninstall | clean]
```

## DESCRIPTION

Guided by the Plan 9 rule that a libc needs nothing beyond raw kernel syscalls, *a64libc* is my messy, purely ego-driven C runtime for 64-bit ARM (aarch64 / AArch64) Linux, wired straight to `svc #0` at 0x400000. It was written for GrapheneOS on a Pixel and lives inside Termux proot Void Linux (musl), where a perfectly good libc already sits on the shelf and I ignore it: every binary is static and never meets musl, glibc, libgcc, or a dynamic loader. It skips `%f` entirely in favor of what I actually wanted to write: a lazy forward-coalescing sbrk/mmap allocator, a 64-byte `.data` line buffer that slashed strace syscall counts, a printf family built on one small formatter, and custom *a64libc-gcc* and *a64libc-tcc* wrappers.

The library is single threaded and is not ISO C stdio: there is no `FILE`, and `fprintf` and `fputs` take a file descriptor. See BUGS.

## BUILDING

A C compiler for aarch64 is needed: gcc or clang, natively on the device. In Termux proot Void:

```
make
```

builds *crt0.o* and *a64libc.a*.

```
make tests
```

also builds the test programs *hello*, *fmt*, and *bf*.

```
make check
```

builds the tests, runs *hello* and *fmt*, and exits nonzero if either reports a failure. `make run` runs *hello* and prints its exit code, `make runbf` feeds a brainf program to *bf*, and `make diag` adds `file`, `size`, and `readelf` output.

To cross build from another machine:

```
make tests CC="clang --target=aarch64-linux-gnu" LDFLAGS_EXTRA=-fuse-ld=lld
```

The Makefile finds the compiler's own freestanding headers and turns on `-fno-tree-loop-distribute-patterns` when the compiler knows it (gcc does, clang does not). It was built and checked with gcc 14.2.1 on Void musl, and cross built with clang 18 and lld.

## INSTALLATION

```
make install
```

copies the library and headers under `PREFIX` (default */opt/a64libc*) and the wrappers to `BINDIR` (default */usr/local/bin*). `DESTDIR` stages the whole tree for packaging. The wrappers are rewritten to the chosen prefix on the way.

```
/opt/a64libc/lib/a64libc.a
/opt/a64libc/lib/crt0.o
/opt/a64libc/lib/a64libc.ld
/opt/a64libc/lib/a64libc-gcc.specs
/opt/a64libc/include/*.h
/usr/local/bin/a64libc-gcc
/usr/local/bin/a64libc-tcc
```

`make uninstall` removes all of it.

## OPTIONS

*a64libc-gcc* compiles and links a static program against the library. Without *-c*, *-S*, or *-E* it puts *crt0.o* before your files and *a64libc.a* after them. Every other option goes to the compiler as given, so `-I`, `-D`, `-W`, and your own `-O` all work. A `-O` of yours wins over the default `-Os`.

```
-c, -S, -E       Compile only. The startup code and the library are
                 not added.
-o out           Write the output to out.
-On, -Os         Set the optimization level. The default is -Os.
```

It always adds

```
-nostdlib -nostdinc -isystem <compiler headers> -I /opt/a64libc/include
-ffreestanding -fno-builtin -fno-stack-protector
-fno-asynchronous-unwind-tables -fno-pie
-static -no-pie -Wl,-z,max-page-size=0x10000 -T a64libc.ld
```

`-nostdinc` keeps Void's musl headers out. The compiler's own *stdarg.h*, *stdbool.h*, and *limits.h* are still found, through `-isystem`.

*a64libc-tcc* compiles one file with tcc and links it with ld. It uses a temporary directory and removes it on exit.

The environment decides which tools and which install are used:

```
A64LIBC    the install prefix, default /opt/a64libc
REALCC     the compiler behind a64libc-gcc, default gcc;
           REALCC="clang --target=aarch64-linux-gnu -fuse-ld=lld" works
REALTCC    the compiler behind a64libc-tcc, default tcc
REALLD     the linker behind a64libc-tcc, default ld
```

*make* takes `CC`, `AR`, `PREFIX`, `BINDIR`, `DESTDIR`, and `LDFLAGS_EXTRA`.

A gcc specs file is installed as well:

```
gcc -specs=/opt/a64libc/lib/a64libc-gcc.specs -static -no-pie prog.c -o prog
```

`-static` and `-no-pie` must be given. A spec cannot switch off the default PIE of a distribution gcc.

## PROGRAM STARTUP

The entry point is `_start`. On entry `sp` points at `argc`, the `argv` pointers, a NULL, the `envp` pointers, and a NULL, as the kernel built them. `_start`

1. clears the frame pointer and link register,
2. keeps `argc`, `argv`, and `envp` in callee-saved registers,
3. aligns `sp` to 16 bytes,
4. clears BSS from `_bss_start` to `_end`, eight bytes at a time,
5. zeroes `errno` and stores `envp` in `environ`,
6. takes the heap base from `brk(0)` and stores it in `__brk_current`,
7. calls `main(argc, argv, envp)`,
8. keeps the result, calls `fflush`, and passes the result to `exit_group`.

A program may declare `extern char **environ;` to reach the environment anywhere.

## SYSTEM CALLS

AArch64 Linux uses `svc #0` with the number in `x8`, the arguments in `x0` to `x5`, and the result in `x0`. A result from -4095 to -1 is `-errno`. Each stub stores the positive value in `errno` and returns -1; anything else is returned untouched.

```
read      63        brk       214
write     64        munmap    215
openat    56        mmap      222
close     57        sys_exit   94   (exit_group)
lseek     62        getpid    172
```

AArch64 has the generic syscall table, so a few things differ from ARM32 and x86-64:

- There is no `open` system call. `open(path, flags, mode)` is `openat(AT_FDCWD, path, flags, mode)`.
- `mmap` is syscall 222 and takes a byte offset, not 4 KB page units.
- `sys_exit` ends the whole process with `exit_group`. `_exit(status)` is a macro for it.
- `sbrk(incr)` returns the old break and keeps the current one in `__brk_current`. Linux `brk` does not report failure: it returns the unchanged break. `sbrk` compares the result with the break it asked for and fails with `ENOMEM` when the kernel did not move it.

`errno` is a plain global `int`. The codes in *errno.h* are the Linux values.

## MEMORY ALLOCATION

`malloc` keeps an implicit singly linked free list. Every block has a 16 byte header: one word that holds the usable size together with the flags (the size is a multiple of 16, so its low bits carry `IS_FREE` and `IS_MMAP`), and a pointer to the next block. User data is 16 byte aligned.

A request of 128 KB or more bypasses the list. It is satisfied with `mmap(MAP_PRIVATE | MAP_ANONYMOUS)`, marked `IS_MMAP`, and handed back to the kernel with `munmap` by `free`. A smaller request walks the list first fit. Before each free block is tested it is merged with the free block that directly follows it, which catches neighbours that were freed in sequence and could not merge at `free` time. A hit is split when the tail can hold a header and 16 bytes of data. If nothing fits, `sbrk` grows the heap by one header and the aligned size.

`free` of an `sbrk` block marks it free and merges it forward at once. Memory is not returned to the kernel; only mmap blocks are. `calloc` checks the multiplication for overflow, and `realloc` returns the same block when the new size fits and the leftover is smaller than a header plus 16 bytes, and otherwise allocates, copies, and frees. `malloc(0)` returns NULL. A request too large to represent fails with `ENOMEM`.

## FORMATTED OUTPUT

All the printf functions run on one formatter that writes through a sink: stdout for `printf` and `vprintf`, a bounded buffer for `snprintf` and `vsnprintf`, and a 256 byte stack buffer for `fprintf`, which then calls `write(fd, ...)` directly. The conversions are

```
%d %i            signed decimal
%u               unsigned decimal
%x %X            hexadecimal, lower or upper case
%c               a character
%s               a string; a NULL pointer prints (null)
%p               0x and the address in lower case hexadecimal
%%               a percent sign
```

with the length modifiers `hh`, `h`, `l`, `ll`, `z`, `j`, and `t`. On LP64 the last five are all 64 bit. `%p` prints the full 64 bit address. There are no flags, width, or precision, and no `%f`, `%e`, `%g`, `%a`, or `%n`. An unknown conversion, such as the `0` of `%08x`, is printed literally and consumes no argument, so every later conversion in the same string reads the wrong argument.

`snprintf` and `vsnprintf` follow C99. The buffer is always NUL terminated when the size is not 0, and the result is the length the whole output would have had, which can be larger than the buffer. A size of 0 or a NULL buffer only counts. `printf` returns the number of characters it produced.

Standard output is line buffered, always, in a 64 byte buffer `stdout_buf` that lives in `.data`. It is flushed when a newline is written, when it is full, at the end of every `printf`, and by *crt0.S* after `main` returns. The result is one `write` per line, not one per character. `fflush` retries on `EINTR` and keeps writing after a short write. `fprintf` and `fputs` write straight to their descriptor and bypass the buffer, which is what makes them suitable for standard error.

## MEMORY LAYOUT

The linker script, installed as *a64libc.ld*, builds a position dependent executable at 0x400000 with two loadable segments and a stack marker:

| Segment | Flags | Sections                                          |
|---------|-------|---------------------------------------------------|
| 1       | R X   | ELF and program headers, .text, .rodata           |
| 2       | R W   | .data, .bss                                       |
| stack   | R W   | PT_GNU_STACK, so the stack is not executable      |

The data segment starts on the next 64 KB boundary of the address space but follows the text directly in the file, so the binary stays small and loads under 4 KB, 16 KB, and 64 KB kernel pages. `_bss_start` and `_end` mark the BSS, both 8 byte aligned. `.comment`, notes, and `.eh_frame` are dropped. The heap starts wherever the kernel put the break.

## LIBRARY

The headers are *errno.h*, *malloc.h*, *stddef.h*, *stdint.h*, *stdio.h*, and *unistd.h*. They declare

```
stdio.h    read write puts putchar printf vprintf snprintf vsnprintf
           fprintf fputs fflush strlen memset memcpy memcmp memmove
           stdout_buf stdout_pos BUFSIZ EOF STDIN_FILENO STDOUT_FILENO
           STDERR_FILENO
malloc.h   malloc free calloc realloc
unistd.h   sys_exit _exit getpid sbrk mmap munmap PROT_READ PROT_WRITE
           MAP_PRIVATE MAP_ANONYMOUS MAP_FAILED
stddef.h   size_t ptrdiff_t wchar_t NULL offsetof
stdint.h   the intN_t and uintN_t types, intptr_t uintptr_t intmax_t
           uintmax_t pid_t ssize_t off_t, and their limits
errno.h    errno and the Linux error codes
```

The types are LP64: `int` is 32 bit and `long`, `size_t`, and pointers are 64 bit. `int64_t` is `long`.

A program built with the wrapper also sees the compiler's *stdarg.h*, *stdbool.h*, and *limits.h*. `va_list` is the compiler's `__builtin_va_list`, so no helper from the library is needed.

## EXIT STATUS

A program that returns from `main` exits with that value after the stdout buffer is flushed. A program that calls `_exit` or `sys_exit` exits at once, without flushing. There is no `exit`, `atexit`, or `abort`.

`make check` exits 0 when every test passes and 1 otherwise. The wrappers exit with the status of the compiler or linker they ran, and *a64libc-tcc* exits 1 when it has no input file.

## DIAGNOSTICS

GNU ld prints

```
warning: .note.gnu.build-id section discarded, --build-id ignored
```

on distributions whose gcc asks for a build id by default. The linker script drops notes, so the warning is harmless and the binary is fine. Passing `-Wl,--build-id=none` should silence it.

A binary that dies with a segmentation fault at once may have been linked as PIE or without the linker script. Use the wrapper, or give `-static -no-pie -T a64libc.ld` yourself. A program built with the specs file but without `-static -no-pie` is linked as PIE by a gcc that defaults to it, which the linker script is not written for.

## IMPLEMENTATION

The library is plain C and AArch64 assembly in the AAPCS64 calling convention. The C files are built with `-mgeneral-regs-only` so the prologue of a variadic function does not spill the vector registers; the library never touches floating point, but your programs may, and the wrapper does not restrict them.

The mem* and string functions are byte loops. A compiler is free to recognise such a loop and replace it by a call to the function it is inside, which gcc may do when it optimizes. The Makefile therefore adds `-fno-tree-loop-distribute-patterns` when the compiler accepts it; clang does not. The wrapper does not add it, because your own code can call the mem* functions of the library.

`__brk_current`, `errno`, and `environ` are ordinary variables in `.data`. The head of the free list and the stdout buffer with its position are the only other state, which is why the library is not thread safe.

AArch64 always has `udiv`, so the number formatters divide by 10 plainly instead of multiplying by a magic constant.

## TESTING

```
make check
```

builds *hello* and *fmt* and runs them.

*test/hello.c* is a tour of the allocator and the formatter: allocation and reuse, splitting, forward coalescing, a large mmap block, `calloc` zeroing, `realloc` growth, `%u`, `%x`, `%X`, `snprintf`, overlapping `memmove`, `memcmp`, an error constant, and `fprintf` to standard error. It prints OK or FAIL on each line, and `make check` fails when it finds a FAIL.

*test/fmt.c* is a harness of 29 checks. It covers the extremes of every integer width with `%d`, `%ld`, `%lu`, `%llu`, `%zu`, `%lx`, and `%lX`, the truncating `hh` and `h`, a 64 bit and a NULL `%p`, a NULL `%s`, an unknown conversion, truncation, the C99 return value of `snprintf` with a size of 0 and with a short buffer, the 16 byte alignment of `malloc`, the failure of a huge `malloc` and a `calloc` overflow, `realloc` of an mmap block, and the startup contract: `argc`, `argv[0]`, the NULL after `argv`, `envp` right behind it, and `getpid`. It prints `ALL OK` and exits 0 when everything passes, otherwise it exits with the number of failures.

*test/bf.c* is a brainf interpreter. `make runbf` runs a program through it.

The tests are built with the same flags as the library. They run on the target, so a cross built `make tests` is checked by copying the binaries to the device.

## FILES

```
crt0.S           startup: stack alignment, BSS clear, argc/argv/envp, brk(0),
                 main, fflush, exit_group
syscalls.S       read write open close lseek getpid brk sbrk mmap munmap
                 sys_exit, errno, __brk_current
malloc.c         free list allocator, mmap for large blocks, calloc, realloc
stdio.c          printf family, puts, putchar, fflush, mem*, strlen,
                 64 byte stdout buffer
include/         errno.h malloc.h stddef.h stdint.h stdio.h unistd.h
linker.ld        layout, installed as a64libc.ld
Makefile         build, tests, install
a64libc-gcc      gcc and clang wrapper
a64libc-tcc      tcc wrapper
gen_specs.sh     writes the gcc specs file at install time
test/hello.c     allocator and formatter tour
test/fmt.c       64 bit formatting, allocator edges, startup contract
test/bf.c        brainf interpreter
get_syscalls.sh  prints the host's syscall numbers through cpp
objdump.sh       disassembles every object file
COPYING          the GNU General Public License, version 3
```

## SEE ALSO

gcc(1), ld(1), tcc(1), elf(5), syscall(2), strace(1)

## BUGS

This is not an ISO C library. There is no `FILE`, `stdout`, `stderr`, `fopen`, or `stdio` layer: `fprintf(int fd, ...)` and `fputs(const char *, int fd)` take a descriptor, so a program written for the standard prototypes needs changes. `strlen` and the mem* functions are declared in *stdio.h*; there is no *string.h*, *stdlib.h*, *ctype.h*, *math.h*, or *time.h*.

`open`, `close`, `lseek`, and `brk` are in the library but no header declares them yet. Declare them yourself, or add the prototypes to *unistd.h*. `environ` is not declared either.

printf has no width, precision, flags, `%f`, `%e`, `%g`, `%a`, or `%n`.

Single threaded: no locking in the allocator or the stdout buffer, and `errno` is a single global.

The free list is singly linked, so coalescing only looks forward. Freeing blocks in reverse address order can leave neighbours unmerged until a later `malloc` walks over them. `free` of a pointer that did not come from `malloc` or that was freed twice is not detected. `realloc` never grows a block in place. Heap memory taken with `sbrk` is never given back.

`fprintf` output is cut at 255 characters. `fflush` takes no argument and flushes standard output only. `fputs` returns the result of `write`.

The mem* and string functions are byte loops, which is correct and not fast. No `abort`, `exit`, `atexit`, `getenv`, `qsort`, `strtol`, `atoi`, or `signal`.

The library is built for the base ARMv8-A instruction set. It does not use BTI, pointer authentication, or MTE, although the CPU may have them.

## LICENSE

Copyright (C) 2026 Ivan Gaydardzhiev. *a64libc* is distributed under the GNU General Public License, version 3 only. See *COPYING*.
