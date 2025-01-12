#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/Body.h"

#include "PhysicsContactListener.h"
#include "../Game/Game.h"

#include "components/debug/Debug.h"

PhysicsContactListener::PhysicsContactListener(Game &game)
	: game(game)
{
	
}

JPH::ValidateResult PhysicsContactListener::OnContactValidate(const JPH::Body &body1, const JPH::Body &body2, JPH::RVec3Arg baseOffset, const JPH::CollideShapeResult &collisionResult)
{
	return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
}

void PhysicsContactListener::OnContactAdded(const JPH::Body &body1, const JPH::Body &body2, const JPH::ContactManifold &manifold, JPH::ContactSettings &settings)
{
	const Player &player = this->game.player;
	const JPH::BodyID playerBodyID = player.physicsCharacter->GetBodyID();
	if ((body1.GetID() != playerBodyID) && (body2.GetID() != playerBodyID))
	{
		return;
	}

	const JPH::Body *playerBody = nullptr;
	const JPH::Body *otherBody = nullptr;
	if (body1.GetID() == playerBodyID)
	{
		playerBody = &body1;
		otherBody = &body2;
	}
	else
	{
		playerBody = &body2;
		otherBody = &body1;
	}

	if (!otherBody->IsSensor())
	{
		return;
	}

	const JPH::BodyID otherBodyID = otherBody->GetID();
	const SceneManager &sceneManager = this->game.sceneManager;
	const VoxelChunkManager &voxelChunkManager = sceneManager.voxelChunkManager;
	const EntityChunkManager &entityChunkManager = sceneManager.entityChunkManager;

	const bool isPlayerVsEntitySensorCollision = entityChunkManager.getEntityFromPhysicsBodyID(otherBodyID) != -1;
	const bool isPlayerVsVoxelSensorCollision = !isPlayerVsEntitySensorCollision;

	// Note that waking up while contacting also counts as contact added, may want to handle? (sleep happens after like 0.75s?)

	if (isPlayerVsEntitySensorCollision)
	{
		DebugLog("Player contacted entity sensor " + std::to_string(otherBodyID.GetIndex()));
	}
	else if (isPlayerVsVoxelSensorCollision)
	{
		DebugLog("Player contacted voxel sensor " + std::to_string(otherBodyID.GetIndex()));
	}
}

void PhysicsContactListener::OnContactPersisted(const JPH::Body &body1, const JPH::Body &body2, const JPH::ContactManifold &manifold, JPH::ContactSettings &settings)
{
	//DebugLog("A contact was persisted between " + std::to_string(body1.GetID().GetIndex()) + " and " + std::to_string(body2.GetID().GetIndex()) + ".");
}

void PhysicsContactListener::OnContactRemoved(const JPH::SubShapeIDPair &subShapePair)
{
	//DebugLog("A contact was removed between " + std::to_string(subShapePair.GetBody1ID().GetIndex()) + " and " + std::to_string(subShapePair.GetBody2ID().GetIndex()) + ".");
}
