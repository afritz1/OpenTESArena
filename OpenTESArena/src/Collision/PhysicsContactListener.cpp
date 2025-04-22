#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/Body.h"

#include "PhysicsContactListener.h"
#include "../Game/Game.h"
#include "../Voxels/VoxelUtils.h"
#include "../World/MapLogicController.h"
#include "../World/MapType.h"

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
	JPH::SubShapeID otherSubShapeID;
	if (body1.GetID() == playerBodyID)
	{
		playerBody = &body1;
		otherBody = &body2;
		otherSubShapeID = manifold.mSubShapeID2;
	}
	else
	{
		playerBody = &body2;
		otherBody = &body1;
		otherSubShapeID = manifold.mSubShapeID1;
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
		JPH::PhysicsSystem &physicsSystem = this->game.physicsSystem;
		GameState &gameState = this->game.gameState;
		const double ceilingScale = gameState.getActiveCeilingScale();

		// Determine which sensor subshape was hit since it's in a compound shape.
		JPH::SubShapeID remainderSubShapeID;
		const JPH::TransformedShape otherSubShapeTransformed = otherBody->GetShape()->GetSubShapeTransformedShape(otherSubShapeID, otherBody->GetCenterOfMassPosition(), otherBody->GetRotation(), JPH::Vec3Arg::sReplicate(1.0f), remainderSubShapeID);
		const JPH::RVec3 otherSubShapePosition = otherSubShapeTransformed.mShapePositionCOM;		
		const CoordDouble3 otherSubShapeCoord = VoxelUtils::worldPointToCoord(WorldDouble3(
			static_cast<SNDouble>(otherSubShapePosition.GetX()),
			static_cast<double>(otherSubShapePosition.GetY()),
			static_cast<WEDouble>(otherSubShapePosition.GetZ())));
		const CoordInt3 otherSubShapeVoxelCoord(otherSubShapeCoord.chunk, VoxelUtils::pointToVoxel(otherSubShapeCoord.point, ceilingScale));
		//DebugLog("Player contacted voxel sensor " + std::to_string(otherBodyID.GetIndex()) + " in chunk (" + otherSubShapeVoxelCoord.chunk.toString() + ") at (" + otherSubShapeVoxelCoord.voxel.toString() + ").");

		TextBox *triggerTextBox = game.getTriggerTextBox();
		DebugAssert(triggerTextBox != nullptr);
		MapLogicController::handleTriggersInVoxel(game, otherSubShapeVoxelCoord, *triggerTextBox);

		const MapType activeMapType = gameState.getActiveMapType();
		if (activeMapType == MapType::Interior)
		{
			const JPH::RVec3 playerBodyPosition = playerBody->GetCenterOfMassPosition();
			const CoordDouble3 playerBodyCoord = VoxelUtils::worldPointToCoord(WorldDouble3(
				static_cast<SNDouble>(playerBodyPosition.GetX()),
				static_cast<double>(playerBodyPosition.GetY()),
				static_cast<WEDouble>(playerBodyPosition.GetZ())));
			const CoordInt3 playerBodyVoxelCoord(playerBodyCoord.chunk, VoxelUtils::pointToVoxel(playerBodyCoord.point, ceilingScale));

			// Have to queue for later due to possible deadlock with player position lookup if displaying world map.
			gameState.queueLevelTransitionCalculation(playerBodyVoxelCoord, otherSubShapeVoxelCoord);			
		}
	}
	else if (isPlayerVsEntitySensorCollision)
	{
		const EntityInstance &entityInst = entityChunkManager.getEntity(otherBodyEntityInstanceID);
		const CoordDouble2 &entityCoord = entityChunkManager.getEntityPosition(entityInst.positionID);
		//DebugLog("Player contacted entity sensor " + std::to_string(otherBodyID.GetIndex()) + " in chunk (" + entityCoord.chunk.toString() + ") at (" + entityCoord.point.toString() + ").");

		// @todo: do we actually care about player + entity sensor collisions? maybe don't need this branch?
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
