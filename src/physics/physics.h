#pragma once
#include "../types.h"

#ifndef PHYSICS_INTERNAL
typedef struct physics_world physics_world;
typedef struct physics_body physics_body;
#endif

struct DebugLine {
	vec3s start;
	vec3s end;
	uint8_t color[4];
};

struct DebugTri {
	vec3s v0, v1, v2;
	uint8_t color[4];
};
