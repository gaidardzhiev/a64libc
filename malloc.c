/*
 * Copyright (C) 2026 Ivan Gaydardzhiev
 * Licensed under the GPL-3.0-only
 */

#include "unistd.h"
#include "malloc.h"
#include "stdio.h"
#include "errno.h"

#define MMAP_THRESHOLD (128ul * 1024ul)
#define HDR_SIZE 16ul
#define ALIGN(n) (((n) + 15ul) & ~15ul)
#define IS_FREE 1ul
#define IS_MMAP 2ul
#define FLAG_MASK 15ul

/*
 * 16 byte header on LP64: one word holding the usable size and the flags
 * (sizes are multiples of 16, so the low 4 bits are free for flags), and the
 * next pointer. User data stays 16 byte aligned.
 */
struct block {
	size_t szf;
	struct block *next;
};

static struct block *heap_head = NULL;

static size_t bsize(struct block *b) {
	return b->szf & ~FLAG_MASK;
}

static size_t bflags(struct block *b) {
	return b->szf & FLAG_MASK;
}

static void bset(struct block *b, size_t size, size_t flags) {
	b->szf = size | flags;
}

static struct block *ptr_to_hdr(void *ptr) {
	return (struct block *)((char *)ptr - HDR_SIZE);
}

static void *hdr_to_ptr(struct block *b) {
	return (char *)b + HDR_SIZE;
}

static void coalesce_forward(struct block *b) {
	struct block *nx = b->next;
	if (!nx || !(bflags(nx) & IS_FREE))
		return;
	if ((char *)hdr_to_ptr(b) + bsize(b) != (char *)nx)
		return;
	bset(b, bsize(b) + HDR_SIZE + bsize(nx), bflags(b));
	b->next = nx->next;
}


void *malloc(size_t size) {
	if (size == 0)
		return NULL;
	if (size > SIZE_MAX - 2 * HDR_SIZE) {
		errno = ENOMEM;
		return NULL;
	}
	size = ALIGN(size);
	if (size >= MMAP_THRESHOLD) {
		size_t total = HDR_SIZE + size;
		struct block *b = mmap(NULL, total, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (b == MAP_FAILED)
			return NULL;
		bset(b, size, IS_MMAP);
		b->next = NULL;
		return hdr_to_ptr(b);
	}
	struct block *prev = NULL;
	struct block *b = heap_head;
	while (b) {
		if (bflags(b) & IS_FREE)
			coalesce_forward(b);
		if ((bflags(b) & IS_FREE) && bsize(b) >= size) {
			if (bsize(b) >= size + HDR_SIZE + 16ul) {
				struct block *rest = (struct block *)((char *)hdr_to_ptr(b) + size);
				bset(rest, bsize(b) - size - HDR_SIZE, IS_FREE);
				rest->next = b->next;
				b->next = rest;
				bset(b, size, 0);
			} else {
				bset(b, bsize(b), 0);
			}
			return hdr_to_ptr(b);
		}
		prev = b;
		b = b->next;
	}
	struct block *nb = sbrk((intptr_t)(HDR_SIZE + size));
	if (nb == (void *)-1)
		return NULL;
	bset(nb, size, 0);
	nb->next = NULL;
	if (prev)
		prev->next = nb;
	else
		heap_head = nb;
	return hdr_to_ptr(nb);
}


void free(void *ptr) {
	if (!ptr)
		return;
	struct block *b = ptr_to_hdr(ptr);
	if (bflags(b) & IS_MMAP) {
		munmap(b, HDR_SIZE + bsize(b));
		return;
	}
	b->szf |= IS_FREE;
	coalesce_forward(b);
}

void *calloc(size_t nmemb, size_t size) {
	if (nmemb && size > (size_t)-1 / nmemb) {
		errno = ENOMEM;
		return NULL;
	}
	size_t total = nmemb * size;
	void *ptr = malloc(total);
	if (ptr)
		memset(ptr, 0, total);
	return ptr;
}

void *realloc(void *ptr, size_t size) {
	if (!ptr)
		return malloc(size);
	if (size == 0) {
		free(ptr);
		return NULL;
	}
	struct block *b = ptr_to_hdr(ptr);
	size_t old_size = bsize(b);
	if (size > SIZE_MAX - 2 * HDR_SIZE) {
		errno = ENOMEM;
		return NULL;
	}
	size_t new_size = ALIGN(size);
	if (new_size <= old_size && old_size - new_size < HDR_SIZE + 16ul)
		return ptr;
	void *np = malloc(size);
	if (!np)
		return NULL;
	size_t copy = old_size < new_size ? old_size : new_size;
	memcpy(np, ptr, copy);
	free(ptr);
	return np;
}
