/*
 * Copyright (C) 2026 Ivan Gaydardzhiev
 * Licensed under the GPL-3.0-only
 */

#include "stdio.h"
#include "unistd.h"
#include "errno.h"

size_t strlen(const char *s) {
	size_t n = 0;
	while (s[n]) n++;
	return n;
}

void *memset(void *s, int c, size_t n) {
	char *p = s;
	while (n--) *p++ = c;
	return s;
}

void *memcpy(void *dest, const void *src, size_t n) {
	char *d = dest;
	const char *s = src;
	while (n--) *d++ = *s++;
	return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
	const unsigned char *a = s1, *b = s2;
	while (n--) {
		if (*a != *b)
			return (int)*a - (int)*b;
		a++; b++;
	}
	return 0;
}

void *memmove(void *dest, const void *src, size_t n) {
	char *d = dest;
	const char *s = src;
	if (d < s || d >= s + n) {
		while (n--) *d++ = *s++;
	} else {
		d += n; s += n;
		while (n--) *--d = *--s;
	}
	return dest;
}

char stdout_buf[BUFSIZ] __attribute__((section(".data")));

unsigned stdout_pos __attribute__((section(".data")));

void fflush(void) {
	size_t off = 0;
	while (off < stdout_pos) {
		ssize_t r = write(1, stdout_buf + off, stdout_pos - off);
		if (r < 0) {
			if (errno == EINTR)
				continue;
			break;
		}
		off += (size_t)r;
	}
	stdout_pos = 0;
}

int putchar(int c) {
	stdout_buf[stdout_pos++] = (char)c;
	if (stdout_pos == BUFSIZ || c == '\n')
		fflush();
	return (unsigned char)c;
}

int puts(const char *s) {
	while (*s) putchar((unsigned char)*s++);
	putchar('\n');
	return 0;
}

static const char _lhex[] = "0123456789abcdef";

static const char _uhex[] = "0123456789ABCDEF";

/* 64 bit: at most 20 decimal digits, plus sign and NUL */
#define NUMBUF 24

static char *fmt_udec(char *end, unsigned long v) {
	*end = '\0';
	do { *--end = '0' + (char)(v % 10u); v /= 10u; } while (v);
	return end;
}

static char *fmt_hex(char *end, unsigned long v, const char *hd) {
	*end = '\0';
	do { *--end = hd[v & 15u]; v >>= 4; } while (v);
	return end;
}

/*
 * One formatter for every printf flavour; output goes through a sink.
 * Supports %s %c %p %d %i %u %x %X %%, with length modifiers hh h l ll z j t
 * (LP64: l, ll, z, j and t are all 64 bit). No width, precision or flags yet.
 * Returns the number of characters the output has (not counting a NUL).
 */
typedef void (*sinkfn)(void *ctx, char c);

static int vformat(sinkfn put, void *ctx, const char *fmt, __builtin_va_list ap) {
	char num[NUMBUF];
	int n = 0;
#define PUT(ch) do { put(ctx, (ch)); n++; } while (0)
#define PUTS(str) do { const char *_s = (str); while (*_s) PUT(*_s++); } while (0)
	while (*fmt) {
		if (*fmt != '%') { PUT(*fmt++); continue; }
		fmt++;
		if (*fmt == 0) break;
		int lm = 0;	/* -2 hh, -1 h, 0 int, 1 64 bit */
		for (;;) {
			char c = *fmt;
			if (c == 'h') { lm = lm == -1 ? -2 : -1; }
			else if (c == 'l' || c == 'z' || c == 'j' || c == 't') { lm = 1; }
			else break;
			fmt++;
		}
		char spec = *fmt;
		if (spec == 0) break;
		fmt++;
		switch (spec) {
		case 's': { const char *s = __builtin_va_arg(ap, const char *); PUTS(s ? s : "(null)"); break; }
		case 'c': { PUT((char)__builtin_va_arg(ap, int)); break; }
		case 'p': {
			unsigned long v = (unsigned long)__builtin_va_arg(ap, void *);
			PUT('0'); PUT('x');
			PUTS(fmt_hex(num + NUMBUF - 1, v, _lhex));
			break;
		}
		case 'd':
		case 'i': {
			long val;
			if (lm == 1) val = __builtin_va_arg(ap, long);
			else if (lm == -1) val = (short)__builtin_va_arg(ap, int);
			else if (lm == -2) val = (signed char)__builtin_va_arg(ap, int);
			else val = __builtin_va_arg(ap, int);
			int neg = val < 0;
			unsigned long uval = neg ? 0ul - (unsigned long)val : (unsigned long)val;
			char *p = fmt_udec(num + NUMBUF - 1, uval);
			if (neg) *--p = '-';
			PUTS(p);
			break;
		}
		case 'u':
		case 'x':
		case 'X': {
			unsigned long v;
			if (lm == 1) v = __builtin_va_arg(ap, unsigned long);
			else if (lm == -1) v = (unsigned short)__builtin_va_arg(ap, unsigned);
			else if (lm == -2) v = (unsigned char)__builtin_va_arg(ap, unsigned);
			else v = __builtin_va_arg(ap, unsigned);
			PUTS(spec == 'u' ? fmt_udec(num + NUMBUF - 1, v)
			    : fmt_hex(num + NUMBUF - 1, v, spec == 'x' ? _lhex : _uhex));
			break;
		}
		case '%': PUT('%'); break;
		default: PUT('%'); PUT(spec); break;
		}
	}
#undef PUTS
#undef PUT
	return n;
}

static void sink_stdout(void *ctx, char c) {
	(void)ctx;
	putchar((unsigned char)c);
}

int vprintf(const char *fmt, __builtin_va_list ap) {
	return vformat(sink_stdout, NULL, fmt, ap);
}

int printf(const char *fmt, ...) {
	__builtin_va_list ap;
	__builtin_va_start(ap, fmt);
	int r = vprintf(fmt, ap);
	__builtin_va_end(ap);
	fflush();
	return r;
}


struct _sbuf {
	char *buf;
	size_t pos;	/* characters produced, including those that did not fit */
	size_t cap;
};

static void sink_sbuf(void *ctx, char c) {
	struct _sbuf *sb = ctx;
	if (sb->pos + 1 < sb->cap) sb->buf[sb->pos] = c;
	sb->pos++;
}

/* C99 semantics: returns the length the full output would have had */
int vsnprintf(char *buf, size_t size, const char *fmt, __builtin_va_list ap) {
	struct _sbuf sb = { buf, 0, buf ? size : 0 };
	int n = vformat(sink_sbuf, &sb, fmt, ap);
	if (sb.cap)
		sb.buf[sb.pos < sb.cap ? sb.pos : sb.cap - 1] = '\0';
	return n;
}

int snprintf(char *buf, size_t size, const char *fmt, ...) {
	__builtin_va_list ap;
	__builtin_va_start(ap, fmt);
	int r = vsnprintf(buf, size, fmt, ap);
	__builtin_va_end(ap);
	return r;
}

int fputs(const char *s, int fd) { return (int)write(fd, s, strlen(s)); }

int fprintf(int fd, const char *fmt, ...) {
	char buf[256];
	__builtin_va_list ap;
	__builtin_va_start(ap, fmt);
	int n = vsnprintf(buf, sizeof(buf), fmt, ap);
	__builtin_va_end(ap);
	if (n > (int)sizeof(buf) - 1) n = (int)sizeof(buf) - 1;
	if (n > 0) write(fd, buf, (size_t)n);
	return n;
}
