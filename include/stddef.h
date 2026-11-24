/*
 * Copyright (C) 2026 Ivan Gaydardzhiev
 * Licensed under the GPL-3.0-only
 */

#ifndef _A64LIBC_STDDEF_H
#define _A64LIBC_STDDEF_H
/* AArch64 Linux, LP64: int 32 bit, long and pointers 64 bit */
typedef unsigned long size_t;
typedef long ptrdiff_t;
typedef unsigned int wchar_t;
#define NULL ((void*)0)
#define offsetof(t, m) ((size_t)&((t *)0)->m)
#endif
