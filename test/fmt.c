/*
 * Copyright (C) 2026 Ivan Gaydardzhiev
 * Licensed under the GPL-3.0-only
 */

#include "stdio.h"
#include "malloc.h"
#include "unistd.h"

static int fails;

static void expect(const char *name, const char *got, const char *want) {
	int ok = strlen(got) == strlen(want) && memcmp(got, want, strlen(want)) == 0;
	if (!ok) fails++;
	printf("%s: \"%s\" %s\n", name, got, ok ? "OK" : "FAIL");
}

static void expect_int(const char *name, long got, long want) {
	if (got != want) fails++;
	printf("%s: %ld %s\n", name, got, got == want ? "OK" : "FAIL");
}

int main(int argc, char **argv, char **envp) {
	char b[64];
	snprintf(b, sizeof b, "%d", INT32_MIN);		expect("%d INT32_MIN", b, "-2147483648");
	snprintf(b, sizeof b, "%ld", INT64_MIN);	expect("%ld INT64_MIN", b, "-9223372036854775808");
	snprintf(b, sizeof b, "%ld", INT64_MAX);	expect("%ld INT64_MAX", b, "9223372036854775807");
	snprintf(b, sizeof b, "%lu", UINT64_MAX);	expect("%lu UINT64_MAX", b, "18446744073709551615");
	snprintf(b, sizeof b, "%llu", UINT64_MAX);	expect("%llu UINT64_MAX", b, "18446744073709551615");
	snprintf(b, sizeof b, "%zu", (size_t)-1);	expect("%zu SIZE_MAX", b, "18446744073709551615");
	snprintf(b, sizeof b, "%lx", 0xdeadbeefcafef00dul);	expect("%lx 64 bit", b, "deadbeefcafef00d");
	snprintf(b, sizeof b, "%lX", 0xdeadbeefcafef00dul);	expect("%lX 64 bit", b, "DEADBEEFCAFEF00D");
	snprintf(b, sizeof b, "%u", 0xffffffffu);	expect("%u 32 bit", b, "4294967295");
	snprintf(b, sizeof b, "%hhd", 300);		expect("%hhd truncates", b, "44");
	snprintf(b, sizeof b, "%hu", 70000);		expect("%hu truncates", b, "4464");
	snprintf(b, sizeof b, "%p", (void *)0x7ffe12345678ul);	expect("%p 64 bit", b, "0x7ffe12345678");
	snprintf(b, sizeof b, "%p", (void *)0);		expect("%p NULL", b, "0x0");
	snprintf(b, sizeof b, "%s", (char *)0);		expect("%s NULL", b, "(null)");
	snprintf(b, sizeof b, "%d%%%c", 7, 'z');	expect("%% and %c", b, "7%z");
	snprintf(b, sizeof b, "%q");			expect("unknown spec", b, "%q");
	snprintf(b, 6, "abcdefghij");			expect("truncate cap 6", b, "abcde");
	expect_int("snprintf full length", snprintf(b, 6, "abcdefghij"), 10);
	expect_int("snprintf size 0", snprintf(0, 0, "%d", 12345), 5);
	expect_int("snprintf ret", snprintf(b, sizeof b, "val=%d", 42), 6);

	void *p[8];
	int i, aligned = 1;
	for (i = 0; i < 8; i++) {
		p[i] = malloc(1 + i * 13);
		if ((unsigned long)p[i] & 15u) aligned = 0;
	}
	expect_int("malloc 16B aligned", aligned, 1);
	for (i = 0; i < 8; i++) free(p[i]);
	void *huge = malloc((size_t)-1);
	expect_int("malloc huge NULL", huge == NULL, 1);
	void *cz = calloc((size_t)1 << 62, 8);
	expect_int("calloc overflow NULL", cz == NULL, 1);
	char *big = malloc(300 * 1024);
	big[0] = 'a'; big[300 * 1024 - 1] = 'z';
	big = realloc(big, 600 * 1024);
	expect_int("realloc mmap keeps", big && big[0] == 'a' && big[300 * 1024 - 1] == 'z', 1);
	free(big);

	expect_int("argc >= 1", argc >= 1, 1);
	expect_int("argv[0] set", argv && argv[0] != 0, 1);
	expect_int("argv NULL term", argv[argc] == 0, 1);
	expect_int("envp after argv", envp == argv + argc + 1, 1);
	expect_int("getpid > 0", getpid() > 0, 1);

	printf("%s\n", fails ? "SOME FAILED" : "ALL OK");
	return fails;
}
