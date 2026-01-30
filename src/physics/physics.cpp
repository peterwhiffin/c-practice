// #define CGLM_USE_ANONYMOUS_STRUCT 1
#include "../types.h"

#include "cglm/types-struct.h"
#include "physics_internal.hpp"
#include <cstdarg>
#include <cstdio>
#include <thread>

#include "../arena.c"

#include <Jolt/Renderer/DebugRenderer.cpp>
#include <Jolt/Renderer/DebugRendererSimple.cpp>
JPH_SUPPRESS_WARNINGS
using namespace JPH;
using namespace JPH::literals;
using namespace std;

#define TIME_STEP 1.0f / 60.0f

void step_physics(struct physics *physics, struct scene *scene, struct game *game, float delta_time)
{
	physics->time_accum += delta_time;

	if (physics->time_accum >= TIME_STEP) {
		physics->time_accum -= TIME_STEP;

		physics->physics_world->physicsSystem->Update(cDeltaTime, cCollisionSteps,
							      physics->physics_world->tempAllocator,
							      physics->physics_world->jobSystem);

		for (int i = 0; i < scene->num_bodies; i++) {
			BodyInterface *bi = physics->physics_world->bodyInterface;
			struct physics_body *body = &scene->bodies[i];
			Vec3 new_pos = bi->GetPosition(body->jolt_body);
			Quat new_rot = bi->GetRotation(body->jolt_body);
			game->set_position(body->entity->transform,
					   (vec3s){ new_pos.GetX(), new_pos.GetY(), new_pos.GetZ() });

			game->set_rotation(body->entity->transform,
					   (versors){ new_rot.GetX(), new_rot.GetY(), new_rot.GetZ(), new_rot.GetW() });
		}
	}

	physics->physics_world->physicsSystem->DrawBodies(physics->physics_world->debug_renderer->mBodyDrawSettings,
							  physics->physics_world->debug_renderer);
}

static void TraceImpl(const char *inFMT, ...)
{
	va_list list;
	va_start(list, inFMT);
	vfprintf(stderr, inFMT, list);
	va_end(list);
}

bool MyObjectLayerPairFilter::ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const
{
	switch (inObject1) {
	case Layers::NON_MOVING:
		return inObject2 == Layers::MOVING; // Non moving only collides with moving
	case Layers::MOVING:
		return true; // Moving collides with everything
	default:
		// JPH_ASSERT(false);
		return false;
	}
}

MyBroadPhaseLayerInterface::MyBroadPhaseLayerInterface()
{
	mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
	mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
}

uint MyBroadPhaseLayerInterface::GetNumBroadPhaseLayers() const
{
	return BroadPhaseLayers::NUM_LAYERS;
}

BroadPhaseLayer MyBroadPhaseLayerInterface::GetBroadPhaseLayer(ObjectLayer inLayer) const
{
	// JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
	return mObjectToBroadPhase[inLayer];
}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
const char *MyBroadPhaseLayerInterface::GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const
{
	switch ((BroadPhaseLayer::Type)inLayer) {
	case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:
		return "NON_MOVING";
	case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:
		return "MOVING";
	default:
		// JPH_ASSERT(false);
		return "INVALID";
	}
}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

/// Class that determines if an object layer can collide with a broadphase layer
bool MyObjectVsBroadPhaseLayerFilter::ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const
{
	switch (inLayer1) {
	case Layers::NON_MOVING:
		return inLayer2 == BroadPhaseLayers::MOVING;
	case Layers::MOVING:
		return true;
	default:
		// JPH_ASSERT(false);
		return false;
	}
}

#ifdef JPH_ENABLE_ASSERTS
// Callback for asserts, connect this to your own assert handler if you have one
static bool AssertFailedImpl(const char *inExpression, const char *inMessage, const char *inFile, uint inLine)
{
	return true;
};

#endif // JPH_ENABLE_ASSERTS

void MyDebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor)
{
	lines.push_back({ inFrom, inTo, inColor });
}

void MyDebugRenderer::DrawTriangle(JPH::RVec3Arg inV1, const JPH::RVec3Arg inV2, const JPH::RVec3Arg inV3,
				   JPH::ColorArg inColor, ECastShadow inCastShadow)
{
	triangles.push_back({ inV1, inV2, inV3, inColor });
}

void MyDebugRenderer::DrawText3D(JPH::RVec3Arg inPosition, const std::string_view &inString, JPH::ColorArg inColor,
				 float inHeight)
{
	// Implement
}

struct physics_body *add_rigidbody(struct physics *physics, struct scene *scene, struct entity *entity, bool is_static)
{
	struct physics_body *r = &scene->bodies[scene->num_bodies];
	entity->body = r;
	r->entity = entity;
	scene->num_bodies++;

	JPH::Vec3 extent = JPH::Vec3(0.5f, 0.5f, 0.5f);

	if (entity->renderer) {
		struct mesh *mesh = entity->renderer->mesh;
		extent.SetX(mesh->extent.x * entity->transform->scale.x);
		extent.SetY(mesh->extent.y * entity->transform->scale.y);
		extent.SetZ(mesh->extent.z * entity->transform->scale.z);
	}

	JPH::BodyInterface *body_interface = physics->physics_world->bodyInterface;
	JPH::BoxShapeSettings box_settings(extent);
	JPH::ShapeSettings::ShapeResult shape_result = box_settings.Create();
	JPH::ShapeRefC shape = shape_result.Get();
	JPH::EMotionType motion_type = is_static ? JPH::EMotionType::Static : JPH::EMotionType::Dynamic;
	ObjectLayer layer = is_static ? Layers::NON_MOVING : Layers::MOVING;
	JPH::BodyCreationSettings body_settings(shape, JPH::RVec3(0.0_r, 0.0_r, 0.0_r), JPH::Quat::sIdentity(),
						motion_type, layer);
	body_settings.mAllowDynamicOrKinematic = true;
	JPH::Body *body = body_interface->CreateBody(body_settings);
	body_interface->AddBody(body->GetID(), JPH::EActivation::Activate);
	entity->body->jolt_body = body->GetID();
	Vec3 inPos = Vec3(entity->transform->pos.x, entity->transform->pos.y, entity->transform->pos.z);
	body_interface->SetPosition(entity->body->jolt_body, inPos, JPH::EActivation::Activate);
	return r;
}

void physics_init(struct physics *physics, struct scene *scene, struct arena *arena)
{
	physics->physics_world = alloc_struct(arena, struct physics_world, 1);
	scene->bodies = alloc_struct(arena, struct physics_body, 4096);
	RegisterDefaultAllocator();
	Trace = TraceImpl;
	JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)
	Factory::sInstance = new Factory();
	RegisterTypes();
	physics_world *physics_world = physics->physics_world;
	physics_world->physicsSystem = new JPH::PhysicsSystem();
	physics_world->tempAllocator = new TempAllocatorImpl(10 * 1024 * 1024);
	physics_world->jobSystem =
		new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);
	physics_world->broad_phase_layer_interface = new MyBroadPhaseLayerInterface();
	physics_world->object_vs_broadphase_layer_filter = new MyObjectVsBroadPhaseLayerFilter();
	physics_world->object_vs_object_layer_filter = new MyObjectLayerPairFilter();
	physics_world->physicsSystem->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints,
					   *physics_world->broad_phase_layer_interface,
					   *physics_world->object_vs_broadphase_layer_filter,
					   *physics_world->object_vs_object_layer_filter);
	physics_world->physicsSystem->SetGravity(JPH::Vec3(0.0f, -18.0f, 0.0f));
	physics_world->bodyInterface = &physics_world->physicsSystem->GetBodyInterface();
	physics_world->debug_renderer = new MyDebugRenderer();
	DebugRenderer::sInstance = physics_world->debug_renderer;
}

extern "C" PETE_API void load_physics_functions(struct physics *physics)
{
	physics->step_physics = step_physics;
	physics->physics_init = physics_init;
	physics->add_rigidbody = add_rigidbody;
}
