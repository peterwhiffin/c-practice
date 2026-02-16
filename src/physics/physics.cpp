// #define CGLM_USE_ANONYMOUS_STRUCT 1
#include "physics.h"
#include "../types.h"

#include "cglm/types-struct.h"
#include "cglm/types.h"
#include "physics_internal.hpp"
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <set>
#include <thread>

#include "../arena.c"
#include <vector>

#include <Jolt/Renderer/DebugRenderer.cpp>
#include <Jolt/Renderer/DebugRendererSimple.cpp>
JPH_SUPPRESS_WARNINGS
using namespace JPH;
using namespace JPH::literals;
using namespace std;

#define TIME_STEP 1.0f / 60.0f

Vec3 vec3_lerp(Vec3 a, Vec3 b, float t)
{
	return a + (b - a) * t;
}

void physics_lerp_transforms(struct physics *phys, struct scene *scene, struct game *game)
{
	BodyInterface *bi = phys->physics_world->bodyInterface;
	float t = phys->time_accum / TIME_STEP;

	for (int i = 0; i < scene->rigidbodies.count; i++) {
		struct rigidbody *rb = &scene->rigidbodies.data[i];
		struct transform *transform = rb->entity->transform;

		Vec3 new_pos = vec3_lerp(rb->prev_pos, rb->current_pos, t);
		Quat new_rot = rb->prev_rot.SLERP(rb->current_rot, t);

		game->set_position(transform, { new_pos.GetX(), new_pos.GetY(), new_pos.GetZ() });
		game->set_rotation(transform, { new_rot.GetX(), new_rot.GetY(), new_rot.GetZ(), new_rot.GetW() });
	}
}

void physics_cache_positions(struct physics *phys, struct scene *scene, struct game *game)
{
	BodyInterface *bi = phys->physics_world->bodyInterface;

	for (int i = 0; i < scene->rigidbodies.count; i++) {
		struct rigidbody *rb = &scene->rigidbodies.data[i];
		struct transform *t = rb->entity->transform;

		game->set_position(t, { rb->current_pos.GetX(), rb->current_pos.GetY(), rb->current_pos.GetZ() });
		game->set_rotation(t, { rb->current_rot.GetX(), rb->current_rot.GetY(), rb->current_rot.GetZ(),
					rb->current_rot.GetW() });

		rb->prev_pos = rb->current_pos;
		rb->prev_rot = rb->current_rot;

		rb->current_pos = bi->GetPosition(rb->jolt_body);
		rb->current_rot = bi->GetRotation(rb->jolt_body);
	}
}

void physics_update(struct physics *physics, struct scene *scene, struct game *game, float delta_time)
{
	physics->time_accum += delta_time;

	if (physics->time_accum >= TIME_STEP) {
		physics->time_accum -= TIME_STEP;

		physics->physics_world->physicsSystem->Update(cDeltaTime, cCollisionSteps,
							      physics->physics_world->tempAllocator,
							      physics->physics_world->jobSystem);

		physics_cache_positions(physics, scene, game);
	} else {
		physics_lerp_transforms(physics, scene, game);
	}

	physics->physics_world->debug_renderer->lines.clear();
	physics->physics_world->debug_renderer->triangles.clear();
	physics->num_lines = 0;
	physics->num_tris = 0;

	if (physics->draw_debug) {
		physics->physics_world->physicsSystem->DrawBodies(
			physics->physics_world->debug_renderer->mBodyDrawSettings,
			physics->physics_world->debug_renderer);

		physics->lines = physics->physics_world->debug_renderer->lines.data();
		physics->num_lines = physics->physics_world->debug_renderer->lines.size();

		physics->tris = physics->physics_world->debug_renderer->triangles.data();
		physics->num_tris = physics->physics_world->debug_renderer->triangles.size();
	}
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
	DebugLine line;
	line.start = { inFrom.GetX(), inFrom.GetY(), inFrom.GetZ() };
	line.end = { inTo.GetX(), inTo.GetY(), inTo.GetZ() };
	line.color[0] = inColor.r;
	line.color[1] = inColor.g;
	line.color[2] = inColor.b;
	line.color[3] = inColor.a;

	lines.push_back(line);
}

void MyDebugRenderer::DrawTriangle(JPH::RVec3Arg inV1, const JPH::RVec3Arg inV2, const JPH::RVec3Arg inV3,
				   JPH::ColorArg inColor, ECastShadow inCastShadow)
{
	DebugTri tri;
	tri.v0 = { inV1.GetX(), inV1.GetY(), inV1.GetZ() };
	tri.v1 = { inV2.GetX(), inV2.GetY(), inV2.GetZ() };
	tri.v2 = { inV3.GetX(), inV3.GetY(), inV3.GetZ() };
	tri.color[0] = inColor.r;
	tri.color[1] = inColor.g;
	tri.color[2] = inColor.b;
	tri.color[3] = inColor.a;
	triangles.push_back(tri);
}

void MyDebugRenderer::DrawText3D(JPH::RVec3Arg inPosition, const std::string_view &inString, JPH::ColorArg inColor,
				 float inHeight)
{
	// Implement
}

void physics_add_force(struct physics *physics, struct rigidbody *body, vec3s force)
{
	// physics->physics_world->bodyInterface->AddForce(body->jolt_body, Vec3(force.x, force.y, force.z),
	// 						JPH::EActivation::Activate);

	physics->physics_world->bodyInterface->SetLinearVelocity(body->jolt_body, Vec3(force.x, force.y, force.z));
}

struct rigidbody *add_sphere_rigidbody(struct physics *physics, struct scene *scene, struct entity *entity,
				       bool is_static)
{
	struct rigidbody *r = &scene->rigidbodies.data[scene->rigidbodies.count];
	entity->body = r;
	r->entity = entity;
	scene->rigidbodies.count++;

	JPH::Vec3 extent = JPH::Vec3(0.5f, 0.5f, 0.5f);

	if (entity->renderer) {
		struct mesh *mesh = entity->renderer->mesh;
		extent.SetX(mesh->extent.x * entity->transform->scale.x);
		extent.SetY(mesh->extent.y * entity->transform->scale.y);
		extent.SetZ(mesh->extent.z * entity->transform->scale.z);
	}

	JPH::BodyInterface *body_interface = physics->physics_world->bodyInterface;
	// JPH::BoxShapeSettings box_settings(extent);
	JPH::SphereShapeSettings sphere_settings(extent.GetX());
	JPH::ShapeSettings::ShapeResult shape_result = sphere_settings.Create();
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

vec3s get_transformed_scale(struct physics *phys, struct rigidbody *body)
{
	TransformedShape tshape = phys->physics_world->bodyInterface->GetTransformedShape(body->jolt_body);
	Vec3 scale = tshape.GetShapeScale();
	AABox aab = tshape.GetWorldSpaceBounds();
	Vec3 extent = aab.GetExtent();
	return { extent.GetX(), extent.GetY(), extent.GetZ() };
}

struct BodySettings physics_get_body_settings(struct physics *phys, struct rigidbody *body)
{
	struct BodySettings settings;
	struct transform *t = body->entity->transform;
	JPH::EMotionType motion = phys->physics_world->bodyInterface->GetMotionType(body->jolt_body);
	JPH::EShapeSubType shape = phys->physics_world->bodyInterface->GetShape(body->jolt_body)->GetSubType();
	TransformedShape tshape = phys->physics_world->bodyInterface->GetTransformedShape(body->jolt_body);
	AABox aab = tshape.GetWorldSpaceBounds();

	auto s = phys->physics_world->bodyInterface->GetShape(body->jolt_body);
	AABox aab2 = s->GetLocalBounds();

	// Vec3 extent = aab.GetExtent();
	Vec3 extent = aab2.GetExtent();

	settings.extents = { extent.GetX() / t->scale.x, extent.GetY() / t->scale.y, extent.GetZ() / t->scale.z };

	switch (motion) {
	case JPH::EMotionType::Static:
		settings.motion = STATIC;
		break;
	case JPH::EMotionType::Kinematic:
		settings.motion = KINEMATIC;
		break;
	case JPH::EMotionType::Dynamic:
		settings.motion = DYNAMIC;
		break;
	}

	switch (shape) {
	case JPH::EShapeSubType::Box:
		settings.shape = BOX;
		break;
	case JPH::EShapeSubType::Sphere:
		settings.shape = SPHERE;
		break;
	case JPH::EShapeSubType::Cylinder:
		settings.shape = CYLINDER;
		break;
	case JPH::EShapeSubType::Capsule:
		settings.shape = CAPSULE;
		break;
	}

	return settings;
}

struct rigidbody *add_rigidbody_box(struct physics *physics, struct scene *scene, struct entity *entity, bool is_static)
{
	struct rigidbody *r = &scene->rigidbodies.data[scene->rigidbodies.count];
	entity->body = r;
	r->entity = entity;
	scene->rigidbodies.count++;

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
	Quat inRot = Quat(entity->transform->rot.x, entity->transform->rot.y, entity->transform->rot.z,
			  entity->transform->rot.w);
	body_interface->SetPositionAndRotation(entity->body->jolt_body, inPos, inRot, JPH::EActivation::Activate);
	// body_interface->SetPosition(entity->body->jolt_body, inPos, JPH::EActivation::Activate);
	return r;
}

struct rigidbody *add_rigidbody(struct physics *physics, struct scene *scene, struct entity *entity,
				struct BodySettings *settings)
{
	struct rigidbody *r = &scene->rigidbodies.data[scene->rigidbodies.count];
	r->settings = *settings;
	entity->body = r;
	r->entity = entity;
	scene->rigidbodies.count++;
	return r;
}

struct entity *rigidbody_get_entity(struct rigidbody *body)
{
	return body->entity;
}

struct rigidbody *rigidbody_get_body(struct scene *scene, size_t index)
{
	return &scene->rigidbodies.data[index];
}

void rigidbody_init(struct physics *physics, struct entity *entity)

{
	struct BodySettings *settings = &entity->body->settings;
	JPH::Vec3 extent = JPH::Vec3(0.5f, 0.5f, 0.5f);

	extent.SetX(settings->extents.x * entity->transform->scale.x);
	extent.SetY(settings->extents.y * entity->transform->scale.y);
	extent.SetZ(settings->extents.z * entity->transform->scale.z);

	JPH::BodyInterface *body_interface = physics->physics_world->bodyInterface;
	JPH::ShapeSettings::ShapeResult shape_result;
	// JPH::ShapeRefC shape = shape_result.Get();
	JPH::EMotionType motion_type;
	ObjectLayer layer;

	switch (settings->shape) {
	case BOX: {
		JPH::BoxShapeSettings box_settings = JPH::BoxShapeSettings(extent);
		shape_result = box_settings.Create();
		break;
	}
	case SPHERE: {
		JPH::SphereShapeSettings sphere_settings = JPH::SphereShapeSettings(extent.GetX());
		shape_result = sphere_settings.Create();
		break;
	}
	case CYLINDER: {
		JPH::CylinderShapeSettings cylinder_settings = JPH::CylinderShapeSettings(extent.GetX(), extent.GetY());
		shape_result = cylinder_settings.Create();
		break;
	}
	case CAPSULE: {
		JPH::CapsuleShapeSettings capsule_settings = JPH::CapsuleShapeSettings(extent.GetX(), extent.GetY());
		shape_result = capsule_settings.Create();
		break;
	}
	}

	switch (settings->motion) {
	case STATIC:
		motion_type = JPH::EMotionType::Static;
		layer = Layers::NON_MOVING;
		break;
	case KINEMATIC:
		motion_type = JPH::EMotionType::Kinematic;
		layer = Layers::MOVING;
		break;
	case DYNAMIC:
		motion_type = JPH::EMotionType::Dynamic;
		layer = Layers::MOVING;
		break;
	}

	JPH::ShapeRefC shape = shape_result.Get();

	JPH::BodyCreationSettings body_settings(shape, JPH::RVec3(0.0_r, 0.0_r, 0.0_r), JPH::Quat::sIdentity(),
						motion_type, layer);

	// JPH::BodyCreationSettings body_settings(shape, inPos, inRot, motion_type, layer);
	body_settings.mAllowDynamicOrKinematic = settings->motion == STATIC ? false : true;
	JPH::Body *body = body_interface->CreateBody(body_settings);
	body_interface->AddBody(body->GetID(), JPH::EActivation::Activate);

	Vec3 inPos = Vec3(entity->transform->pos.x, entity->transform->pos.y, entity->transform->pos.z);
	Quat inRot = Quat(entity->transform->rot.x, entity->transform->rot.y, entity->transform->rot.z,
			  entity->transform->rot.w);
	entity->body->jolt_body = body->GetID();
	body_interface->SetPosition(entity->body->jolt_body, inPos, JPH::EActivation::Activate);
	body_interface->SetRotation(entity->body->jolt_body, inRot, JPH::EActivation::Activate);
	// body_interface->SetPositionAndRotation(entity->body->jolt_body, inPos, inRot, JPH::EActivation::Activate);
}

void physics_init(struct physics *physics, struct scene *scene, struct arena *arena)
{
	physics->physics_world = alloc_struct(arena, struct physics_world, 1);
	scene->rigidbodies.data = alloc_struct(arena, struct rigidbody, 4096);
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
	physics_world->debug_renderer->mBodyDrawSettings.mDrawShape = true;
	physics_world->debug_renderer->mBodyDrawSettings.mDrawShapeWireframe = true;
	physics_world->debug_renderer->mBodyDrawSettings.mDrawShapeColor =
		JPH::BodyManager::EShapeColor::MotionTypeColor;

	// drawSettings.mDrawShape = true;
	// drawSettings.mDrawShapeWireframe = true;
	// drawSettings.mDrawShapeColor = JPH::BodyManager::EShapeColor::MotionTypeColor;
	DebugRenderer::sInstance = physics_world->debug_renderer;
}

void physics_remove_rigidbody(struct scene *scene, struct entity *entity)
{
	struct rigidbody *last_body = &scene->rigidbodies.data[scene->rigidbodies.count - 1];

	scene->physics->physics_world->bodyInterface->RemoveBody(entity->body->jolt_body);
	scene->physics->physics_world->bodyInterface->DestroyBody(entity->body->jolt_body);

	if (last_body != entity->body) {
		struct entity *swap_entity = last_body->entity;
		struct rigidbody temp_body = *swap_entity->body;
		swap_entity->body = entity->body;
		*swap_entity->body = temp_body;
	}
	scene->rigidbodies.count--;
}

extern "C" PETE_API void load_physics_functions(struct physics *physics)
{
	physics->step_physics = physics_update;
	physics->physics_init = physics_init;
	physics->add_rigidbody = add_rigidbody;
	physics->add_rigidbody_box = add_rigidbody_box;
	physics->add_sphere_rigidbody = add_sphere_rigidbody;
	physics->physics_add_force = physics_add_force;
	physics->get_transformed_scale = get_transformed_scale;
	physics->physics_get_body_settings = physics_get_body_settings;
	physics->rigidbody_init = rigidbody_init;
	physics->rigidbody_get_entity = rigidbody_get_entity;
	physics->rigidbody_get_body = rigidbody_get_body;
	physics->physics_remove_rigidbody = physics_remove_rigidbody;
}
