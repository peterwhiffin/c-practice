#pragma once
#include "types.h"
#include <stddef.h>

struct arena *get_new_arena(size_t size);
void *alloc(struct arena *arena, size_t size, size_t alignment);
void arena_free(struct arena *arena);

#define alloc_struct(arena, type, count) (type *)alloc((arena), sizeof(type) * (count), _Alignof(type))
