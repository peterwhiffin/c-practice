#pragma once
#include "../types.h"
#include "cglm/types-struct.h"

#ifndef PHYSICS_INTERNAL
typedef struct physics_world physics_world;
typedef struct physics_body physics_body;
#endif

enum body_shape { BOX, SPHERE, CYLINDER, CAPSULE };
enum body_motion { STATIC, KINEMATIC, DYNAMIC };

struct DebugLine {
	vec3s start;
	vec3s end;
	uint8_t color[4];
};

struct DebugTri {
	vec3s v0, v1, v2;
	uint8_t color[4];
};

struct BodySettings {
	enum body_shape shape;
	enum body_motion motion;
	vec3s extents;
};
