#pragma once
#include "../types.h"

void create_mesh_infos(struct resources *res, struct renderer *ren, char *filename, struct mesh_info *mesh_infos,
		       size_t *num_mesh_infos, float import_scale);
