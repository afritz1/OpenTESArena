#include "PhysicsLayer.h"

#include "components/debug/Debug.h"

bool PhysicsObjectLayerPairFilter::ShouldCollide(JPH::ObjectLayer object1, JPH::ObjectLayer object2) const
{
	switch (object1)
	{
	case PhysicsLayers::NON_MOVING:
		return object2 == PhysicsLayers::MOVING;
	case PhysicsLayers::MOVING:
		return (object2 == PhysicsLayers::NON_MOVING) || (object2 == PhysicsLayers::MOVING) || (object2 == PhysicsLayers::SENSOR);
	case PhysicsLayers::SENSOR:
		return (object2 == PhysicsLayers::MOVING);
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(object1));
	}
}

PhysicsBroadPhaseLayerInterface::PhysicsBroadPhaseLayerInterface()
{
	this->objectToBroadPhase[PhysicsLayers::NON_MOVING] = PhysicsBroadPhaseLayers::NON_MOVING;
	this->objectToBroadPhase[PhysicsLayers::MOVING] = PhysicsBroadPhaseLayers::MOVING;
	this->objectToBroadPhase[PhysicsLayers::SENSOR] = PhysicsBroadPhaseLayers::SENSOR;
}

uint32_t PhysicsBroadPhaseLayerInterface::GetNumBroadPhaseLayers() const
{
	return PhysicsBroadPhaseLayers::NUM_LAYERS;
}

JPH::BroadPhaseLayer PhysicsBroadPhaseLayerInterface::GetBroadPhaseLayer(JPH::ObjectLayer layer) const
{
	DebugAssertIndex(this->objectToBroadPhase, layer);
	return this->objectToBroadPhase[layer];
}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
const char *PhysicsBroadPhaseLayerInterface::GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const
{
	const JPH::BroadPhaseLayer::Type layerType = static_cast<JPH::BroadPhaseLayer::Type>(layer);

	switch (layerType)
	{
	case static_cast<JPH::BroadPhaseLayer::Type>(PhysicsBroadPhaseLayers::NON_MOVING):
		return "NON_MOVING";
	case static_cast<JPH::BroadPhaseLayer::Type>(PhysicsBroadPhaseLayers::MOVING):
		return "MOVING";
	case static_cast<JPH::BroadPhaseLayer::Type>(PhysicsBroadPhaseLayers::SENSOR):
		return "SENSOR";
	default:
		DebugNotImplementedMsg(std::to_string(layerType));
		return nullptr;
	}
}
#endif

bool PhysicsObjectVsBroadPhaseLayerFilter::ShouldCollide(JPH::ObjectLayer layer1, JPH::BroadPhaseLayer layer2) const
{
	switch (layer1)
	{
	case PhysicsLayers::NON_MOVING:
		return layer2 == PhysicsBroadPhaseLayers::MOVING;
	case PhysicsLayers::MOVING:
		return (layer2 == PhysicsBroadPhaseLayers::NON_MOVING) || (layer2 == PhysicsBroadPhaseLayers::MOVING) || (layer2 == PhysicsBroadPhaseLayers::SENSOR);
	case PhysicsLayers::SENSOR:
		return layer2 == PhysicsBroadPhaseLayers::MOVING;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(layer1));
	}
}
