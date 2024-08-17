#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/BodyID.h"

#include "PhysicsBodyActivationListener.h"

#include "components/debug/Debug.h"

void PhysicsBodyActivationListener::OnBodyActivated(const JPH::BodyID &bodyID, uint64_t bodyUserData)
{
	DebugLog("Body " + std::to_string(bodyID.GetIndex()) + " got activated.");
}

void PhysicsBodyActivationListener::OnBodyDeactivated(const JPH::BodyID &bodyID, uint64_t bodyUserData)
{
	DebugLog("Body " + std::to_string(bodyID.GetIndex()) + " went to sleep.");
}
