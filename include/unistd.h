/*
 * Copyright (C) 2026 Ivan Gaydardzhiev
 * Licensed under the GPL-3.0-only
 */

#ifndef _A64LIBC_UNISTD_H
#define _A64LIBC_UNISTD_H
#include "stddef.h"
#include "stdint.h"
extern int sys_exit(int status);
extern pid_t getpid(void);
extern void *sbrk(intptr_t incr);
/* aarch64 mmap takes a byte offset (not 4KB page units as ARM32 mmap2) */
extern void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
extern int munmap(void *addr, size_t len);
#define _exit(status) sys_exit(status)
#define PROT_READ  1
#define PROT_WRITE 2
#define MAP_PRIVATE   0x02
#define MAP_ANONYMOUS 0x20
#define MAP_FAILED ((void *)-1)
#endif
