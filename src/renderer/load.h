#pragma once
#include "glad/glad.h"
#include "../types.h"

const char *read_file(char *filename);
struct texture *find_texture(struct resources *res, const char *name);
struct material *find_material(struct resources *res, const char *mat_name);
struct material *get_new_material(struct resources *res, struct renderer *ren, const char *mat_name);
