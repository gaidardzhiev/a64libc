/*
 * Copyright (C) 2026 Ivan Gaydardzhiev
 * Licensed under the GPL-3.0-only
 *
 * AArch64 Linux syscalls: number in x8, args in x0-x5, svc #0, result in x0.
 * Return values in -4095..-1 are -errno.
 */

.section .text

/* x0 = -errno on entry to .Lsyserr, +errno on entry to .Lseterr */
.Lsyserr:
neg x0, x0
.Lseterr:
adrp x1, errno
str w0, [x1, :lo12:errno]
mov x0, #-1
ret

.macro SYSCALL name, nr
.global \name
.type \name, %function
\name:
mov x8, #\nr
svc #0
cmn x0, #4096
b.hi .Lsyserr
ret
.size \name, . - \name
.endm

SYSCALL read, 63
SYSCALL write, 64
SYSCALL close, 57
SYSCALL lseek, 62
SYSCALL getpid, 172
SYSCALL brk, 214
SYSCALL sys_exit, 94
SYSCALL munmap, 215
SYSCALL mmap, 222

/* open(path, flags, mode) -> openat(AT_FDCWD, path, flags, mode); no open syscall on aarch64 */
.global open
.type open, %function
open:
mov x3, x2
mov x2, x1
mov x1, x0
mov x0, #-100
mov x8, #56
svc #0
cmn x0, #4096
b.hi .Lsyserr
ret
.size open, . - open

/* sbrk(incr): old break on success, -1 and errno on failure.
 * Linux brk() does not return an error: on failure it returns the unchanged
 * break, so success is checked by comparing against the requested break. */
.global sbrk
.type sbrk, %function
sbrk:
adrp x1, __brk_current
ldr x2, [x1, :lo12:__brk_current]
add x3, x2, x0
mov x0, x3
mov x8, #214
svc #0
cmp x0, x3
b.lo 1f
str x0, [x1, :lo12:__brk_current]
mov x0, x2
ret
1:
mov x0, #12
b .Lseterr
.size sbrk, . - sbrk

.section .data
.balign 8
.global errno
.type errno, %object
errno: .word 0
.balign 8
.global __brk_current
.type __brk_current, %object
__brk_current: .xword 0
