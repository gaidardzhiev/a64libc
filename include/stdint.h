/*
 * Copyright (C) 2026 Ivan Gaydardzhiev
 * Licensed under the GPL-3.0-only
 */

#ifndef _A64LIBC_STDINT_H
#define _A64LIBC_STDINT_H
/* LP64: int64_t is long, as in glibc and musl on aarch64 */
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef signed long int64_t;
typedef unsigned long uint64_t;
typedef int64_t intptr_t;
typedef uint64_t uintptr_t;
typedef int64_t intmax_t;
typedef uint64_t uintmax_t;
/* POSIX types the syscall layer needs */
typedef int32_t pid_t;
typedef int64_t ssize_t;
typedef int64_t off_t;

#define INT8_MAX 127
#define INT8_MIN (-INT8_MAX - 1)
#define UINT8_MAX 255
#define INT16_MAX 32767
#define INT16_MIN (-INT16_MAX - 1)
#define UINT16_MAX 65535
#define INT32_MAX 2147483647
#define INT32_MIN (-INT32_MAX - 1)
#define UINT32_MAX 4294967295u
#define INT64_MAX 9223372036854775807l
#define INT64_MIN (-INT64_MAX - 1)
#define UINT64_MAX 18446744073709551615ul
#define INTPTR_MAX INT64_MAX
#define INTPTR_MIN INT64_MIN
#define UINTPTR_MAX UINT64_MAX
#define INTMAX_MAX INT64_MAX
#define INTMAX_MIN INT64_MIN
#define UINTMAX_MAX UINT64_MAX
#define SIZE_MAX UINT64_MAX
#define SSIZE_MAX INT64_MAX
#endif
