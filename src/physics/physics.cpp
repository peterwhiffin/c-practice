#include "../types.h"

#define TIME_STEP 1.0f / 60.0f

void step_physics(struct physics *physics, float delta_time)
{
	physics->time_accum += delta_time;

	if (physics->time_accum >= TIME_STEP) {
		physics->time_accum -= TIME_STEP;
	}
}

void physics_init(struct physics *physics)
{
}

extern "C" PETE_API void load_physics_functions(struct physics *physics)
{
	physics->step_physics = step_physics;
}
