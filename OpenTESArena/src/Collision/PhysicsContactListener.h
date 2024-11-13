#ifndef PHYSICS_CONTACT_LISTENER_H
#define PHYSICS_CONTACT_LISTENER_H

#include "Jolt/Jolt.h"
#include "Jolt/Physics/Collision/ContactListener.h"

class PhysicsContactListener : public JPH::ContactListener
{
public:
	JPH::ValidateResult OnContactValidate(const JPH::Body &body1, const JPH::Body &body2, JPH::RVec3Arg baseOffset, const JPH::CollideShapeResult &collisionResult) override;
	void OnContactAdded(const JPH::Body &body1, const JPH::Body &body2, const JPH::ContactManifold &manifold, JPH::ContactSettings &settings) override;
	void OnContactPersisted(const JPH::Body &body1, const JPH::Body &body2, const JPH::ContactManifold &manifold, JPH::ContactSettings &settings) override;
	void OnContactRemoved(const JPH::SubShapeIDPair &subShapePair) override;
};

#endif
