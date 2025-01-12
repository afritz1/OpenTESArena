#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/Body.h"

#include "PhysicsContactListener.h"
#include "../Game/Game.h"
#include "../Voxels/VoxelUtils.h"

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

	const EntityInstanceID otherBodyEntityInstanceID = entityChunkManager.getEntityFromPhysicsBodyID(otherBodyID);
	const bool isPlayerVsEntitySensorCollision = otherBodyEntityInstanceID != -1;
	const bool isPlayerVsVoxelSensorCollision = !isPlayerVsEntitySensorCollision;

	if (isPlayerVsVoxelSensorCollision)
	{
		const GameState &gameState = this->game.gameState;
		const double ceilingScale = gameState.getActiveCeilingScale();

		const JPH::RVec3 otherBodyPosition = otherBody->GetCenterOfMassPosition();
		const CoordDouble3 otherBodyCoord = VoxelUtils::worldPointToCoord(WorldDouble3(
			static_cast<SNDouble>(otherBodyPosition.GetX()),
			static_cast<double>(otherBodyPosition.GetY()),
			static_cast<WEDouble>(otherBodyPosition.GetZ())));
		const CoordInt3 otherBodyVoxelCoord(otherBodyCoord.chunk, VoxelUtils::pointToVoxel(otherBodyCoord.point, ceilingScale));
		DebugLog("Player contacted voxel sensor " + std::to_string(otherBodyID.GetIndex()) + " in chunk (" + otherBodyVoxelCoord.chunk.toString() + ") at (" + otherBodyVoxelCoord.voxel.toString() + ").");
	}
	else if (isPlayerVsEntitySensorCollision)
	{
		const EntityInstance &entityInst = entityChunkManager.getEntity(otherBodyEntityInstanceID);
		const CoordDouble2 &entityCoord = entityChunkManager.getEntityPosition(entityInst.positionID);
		DebugLog("Player contacted entity sensor " + std::to_string(otherBodyID.GetIndex()) + " in chunk (" + entityCoord.chunk.toString() + ") at (" + entityCoord.point.toString() + ").");
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
