#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/Body.h"

#include "PhysicsContactListener.h"

#include "components/debug/Debug.h"

JPH::ValidateResult PhysicsContactListener::OnContactValidate(const JPH::Body &body1, const JPH::Body &body2, JPH::RVec3Arg baseOffset, const JPH::CollideShapeResult &collisionResult)
{
	//DebugLog("Contact validate callback for " + std::to_string(body1.GetID().GetIndex()) + " and " + std::to_string(body2.GetID().GetIndex()) + ".");
	return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
}

void PhysicsContactListener::OnContactAdded(const JPH::Body &body1, const JPH::Body &body2, const JPH::ContactManifold &manifold, JPH::ContactSettings &settings)
{
	//DebugLog("A contact was added between " + std::to_string(body1.GetID().GetIndex()) + " and " + std::to_string(body2.GetID().GetIndex()) + ".");
}

void PhysicsContactListener::OnContactPersisted(const JPH::Body &body1, const JPH::Body &body2, const JPH::ContactManifold &manifold, JPH::ContactSettings &settings)
{
	//DebugLog("A contact was persisted between " + std::to_string(body1.GetID().GetIndex()) + " and " + std::to_string(body2.GetID().GetIndex()) + ".");
}

void PhysicsContactListener::OnContactRemoved(const JPH::SubShapeIDPair &subShapePair)
{
	//DebugLog("A contact was removed between " + std::to_string(subShapePair.GetBody1ID().GetIndex()) + " and " + std::to_string(subShapePair.GetBody2ID().GetIndex()) + ".");
}
