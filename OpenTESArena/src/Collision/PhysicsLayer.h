#ifndef PHYSICS_LAYER_H
#define PHYSICS_LAYER_H

#include <cstdint>

#include "Jolt/Jolt.h"
#include "Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h"
#include "Jolt/Physics/Collision/ObjectLayer.h"

namespace PhysicsLayers
{
	static constexpr JPH::ObjectLayer NON_MOVING = 0;
	static constexpr JPH::ObjectLayer MOVING = 1;
	static constexpr JPH::ObjectLayer SENSOR = 2;
	static constexpr JPH::ObjectLayer NUM_LAYERS = 3;
};

namespace PhysicsBroadPhaseLayers
{
	static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
	static constexpr JPH::BroadPhaseLayer MOVING(1);
	static constexpr JPH::BroadPhaseLayer SENSOR(2);
	static constexpr uint32_t NUM_LAYERS = 3;
};

class PhysicsObjectLayerPairFilter : public JPH::ObjectLayerPairFilter
{
public:
	bool ShouldCollide(JPH::ObjectLayer object1, JPH::ObjectLayer object2) const override;
};

class PhysicsBroadPhaseLayerInterface final : public JPH::BroadPhaseLayerInterface
{
private:
	JPH::BroadPhaseLayer objectToBroadPhase[PhysicsLayers::NUM_LAYERS];
public:
	PhysicsBroadPhaseLayerInterface();

	uint32_t GetNumBroadPhaseLayers() const override;
	JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override;

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	const char *GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override;
#endif
};

class PhysicsObjectVsBroadPhaseLayerFilter : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
	bool ShouldCollide(JPH::ObjectLayer layer1, JPH::BroadPhaseLayer layer2) const override;
};

#endif
