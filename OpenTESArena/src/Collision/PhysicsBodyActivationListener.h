#ifndef PHYSICS_BODY_ACTIVATION_LISTENER_H
#define PHYSICS_BODY_ACTIVATION_LISTENER_H

#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/BodyActivationListener.h"

class PhysicsBodyActivationListener : public JPH::BodyActivationListener
{
public:
	void OnBodyActivated(const JPH::BodyID &bodyID, uint64_t bodyUserData) override;
	void OnBodyDeactivated(const JPH::BodyID &bodyID, uint64_t bodyUserData) override;
};

#endif
