#pragma once
#include <cstdint>
#define PHYSICS_INTERNAL
#define JPH_DEBUG_RENDERER
#include <Jolt/Jolt.h>
#include <Jolt/Math/Real.h>
#include <Jolt/Math/Float2.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Renderer/DebugRendererSimple.h>
#include <vector>
#include "cglm/struct.h"
#include "physics.h"

namespace Layers
{
static constexpr JPH::ObjectLayer NON_MOVING = 0;
static constexpr JPH::ObjectLayer MOVING = 1;
static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
}; // namespace Layers

namespace BroadPhaseLayers
{
static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
static constexpr JPH::BroadPhaseLayer MOVING(1);
static constexpr JPH::uint NUM_LAYERS(2);
}; // namespace BroadPhaseLayers

const JPH::uint cMaxBodies = 65536;
const JPH::uint cNumBodyMutexes = 0;
const JPH::uint cMaxBodyPairs = 65536;
const JPH::uint cMaxContactConstraints = 10240;
const JPH::uint cCollisionSteps = 1;
constexpr double cDeltaTime = 1.0 / 60.0;

struct rigidbody {
	struct entity *entity;
	struct BodySettings settings;
	JPH::Vec3 prev_pos;
	JPH::Vec3 current_pos;
	JPH::Quat prev_rot;
	JPH::Quat current_rot;
	class JPH::BodyID jolt_body;
};

class MyObjectLayerPairFilter : public JPH::ObjectLayerPairFilter {
    public:
	virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override;
};

class MyBroadPhaseLayerInterface final : public JPH::BroadPhaseLayerInterface {
    public:
	MyBroadPhaseLayerInterface();
	virtual JPH::uint GetNumBroadPhaseLayers() const override;
	virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override;

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	virtual const char *GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override;
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

    private:
	JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class MyObjectVsBroadPhaseLayerFilter : public JPH::ObjectVsBroadPhaseLayerFilter {
    public:
	virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override;
};

class MyDebugRenderer : public JPH::DebugRendererSimple {
    public:
	std::vector<DebugLine> lines;
	std::vector<DebugTri> triangles;
	JPH::BodyManager::DrawSettings mBodyDrawSettings; // Settings for how to draw bodies from the body manager

	void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;
	void DrawTriangle(JPH::RVec3Arg inV1, const JPH::RVec3Arg inV2, const JPH::RVec3Arg inV3, JPH::ColorArg inColor,
			  ECastShadow inCastShadow) override;
	void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view &inString, JPH::ColorArg inColor,
			float inHeight) override;
	void Clear();
};

struct physics_world {
	JPH::PhysicsSystem *physicsSystem;
	JPH::BodyInterface *bodyInterface;
	JPH::TempAllocatorImpl *tempAllocator;
	JPH::JobSystemThreadPool *jobSystem;
	MyBroadPhaseLayerInterface *broad_phase_layer_interface;
	MyObjectVsBroadPhaseLayerFilter *object_vs_broadphase_layer_filter;
	MyObjectLayerPairFilter *object_vs_object_layer_filter;
	MyDebugRenderer *debug_renderer;
};
