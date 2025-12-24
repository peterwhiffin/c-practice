#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "arena.h"

struct arena *get_new_arena(size_t size)
{
	struct arena *a = malloc(sizeof(*a));
	a->pos = 0;
	a->size = size;
	a->mem = malloc(size);
	if (a->mem == NULL) {
		printf("ERROR::MALLOC::%s\n", strerror(errno));
	}

	memset(a->mem, 0, size);

	return a;
}

void *alloc(struct arena *arena, size_t size, size_t alignment)
{
	size_t p = (arena->pos + alignment - 1) & ~(alignment - 1);
	void *s = arena->mem + p;
	arena->pos = p + size;

	printf("arena size: %lu - arena pos: %lu\n", arena->size, arena->pos);
	return s;
}

void arena_free(struct arena *arena)
{
	free(arena->mem);
}
