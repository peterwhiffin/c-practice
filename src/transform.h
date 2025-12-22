#pragma once
#include "types.h"

float vec3_magnitude(vec3s v);
vec3s vec3_normalize(vec3s v);
vec3s get_up(struct transform *t);
vec3s get_forward(struct transform *t);
vec3s get_right(struct transform *t);

void set_position(struct transform *t, vec3s pos);
void set_rotation(struct transform *t, versors rot);
void set_scale(struct transform *t, vec3s scale);
