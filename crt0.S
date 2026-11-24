/*
 * Copyright (C) 2026 Ivan Gaydardzhiev
 * Licensed under the GPL-3.0-only
 *
 * AArch64 Linux entry. On entry sp -> argc, argv[0..argc-1], NULL, envp..., NULL
 */

.section .text
.global _start
.type _start, %function
_start:
mov x29, #0
mov x30, #0
// argc, argv, envp, kept in callee-saved registers across the setup below
ldr x19, [sp]
add x20, sp, #8
add x21, x20, x19, lsl #3
add x21, x21, #8
mov x3, sp
and x3, x3, #-16
mov sp, x3
// clear BSS: _bss_start and _end are 8 byte aligned by the linker script
adrp x0, _bss_start
add x0, x0, :lo12:_bss_start
adrp x1, _end
add x1, x1, :lo12:_end
1:
cmp x0, x1
b.hs 2f
str xzr, [x0], #8
b 1b
2:
adrp x3, errno
str wzr, [x3, :lo12:errno]
adrp x3, environ
str x21, [x3, :lo12:environ]
// heap base: brk(0)
mov x0, #0
mov x8, #214
svc #0
adrp x3, __brk_current
str x0, [x3, :lo12:__brk_current]
mov x0, x19
mov x1, x20
mov x2, x21
bl main
mov w19, w0
bl fflush
mov w0, w19
mov x8, #94
svc #0
b .
.size _start, . - _start

.section .data
.balign 8
.global environ
.type environ, %object
environ: .xword 0
