#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/Body.h"

#include "PhysicsContactListener.h"
#include "../Combat/CombatLogic.h"
#include "../Game/Game.h"
#include "../Voxels/VoxelUtils.h"
#include "../World/MapLogic.h"
#include "../World/MapType.h"

#include "components/debug/Debug.h"

namespace
{
	void OnPlayerVsVoxelContactAdded(const JPH::Body &playerBody, const JPH::Body &voxelBody, JPH::SubShapeID voxelSubShapeID, bool isVoxelSensor, Game &game)
	{
		if (!isVoxelSensor)
		{
			return;
		}

		const VoxelChunkManager &voxelChunkManager = game.sceneManager.voxelChunkManager;
		JPH::PhysicsSystem &physicsSystem = game.physicsSystem;
		GameState &gameState = game.gameState;
		const double ceilingScale = gameState.getActiveCeilingScale();

		// Determine which sensor subshape was hit since it's in a compound shape.
		const JPH::Vec3 otherBodyScale = JPH::Vec3::sReplicate(1.0f);
		JPH::SubShapeID remainderSubShapeID;
		const JPH::TransformedShape otherSubShapeTransformed = voxelBody.GetShape()->GetSubShapeTransformedShape(voxelSubShapeID, voxelBody.GetCenterOfMassPosition(), voxelBody.GetRotation(), otherBodyScale, remainderSubShapeID);
		const JPH::RVec3 otherSubShapePosition = otherSubShapeTransformed.mShapePositionCOM;
		const CoordDouble3 otherSubShapeCoord = VoxelUtils::worldPointToCoord(WorldDouble3(
			static_cast<SNDouble>(otherSubShapePosition.GetX()),
			static_cast<double>(otherSubShapePosition.GetY()),
			static_cast<WEDouble>(otherSubShapePosition.GetZ())));
		const CoordInt3 otherSubShapeVoxelCoord(otherSubShapeCoord.chunk, VoxelUtils::pointToVoxel(otherSubShapeCoord.point, ceilingScale));
		MapLogic::handleTriggersInVoxel(game, otherSubShapeVoxelCoord);

		const MapType activeMapType = gameState.getActiveMapType();
		if (activeMapType == MapType::Interior)
		{
			const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(otherSubShapeVoxelCoord.chunk);
			const VoxelInt3 otherSubShapeVoxel = otherSubShapeVoxelCoord.voxel;

			VoxelTransitionDefID transitionDefID;
			if (!voxelChunk.tryGetTransitionDefID(otherSubShapeVoxel.x, otherSubShapeVoxel.y, otherSubShapeVoxel.z, &transitionDefID))
			{
				return;
			}

			const JPH::RVec3 playerBodyPosition = playerBody.GetCenterOfMassPosition();
			const CoordDouble3 playerBodyCoord = VoxelUtils::worldPointToCoord(WorldDouble3(
				static_cast<SNDouble>(playerBodyPosition.GetX()),
				static_cast<double>(playerBodyPosition.GetY()),
				static_cast<WEDouble>(playerBodyPosition.GetZ())));
			const CoordInt3 playerBodyVoxelCoord(playerBodyCoord.chunk, VoxelUtils::pointToVoxel(playerBodyCoord.point, ceilingScale));

			// Have to queue for later due to possible deadlock with player position lookup if displaying world map.
			gameState.queueLevelTransitionCalculation(playerBodyVoxelCoord, otherSubShapeVoxelCoord);
		}
	}

	// @todo will use this eventually for player getting hit by spells
	/*void OnPlayerVsEntityContactAdded(JPH::BodyID playerBodyID, JPH::BodyID entityBodyID, EntityInstanceID entityInstID, const EntityChunkManager &entityChunkManager)
	{
		const EntityInstance &entityInst = entityChunkManager.entities.get(entityInstID);
		const WorldDouble3 entityPosition = entityChunkManager.positions.get(entityInst.positionID);		
	}*/

	void OnBowProjectileVsVoxelContactAdded(const JPH::Body &projectileBody, EntityInstanceID projectileInstID, const JPH::Body &voxelBody, JPH::SubShapeID voxelSubShapeID, bool isVoxelSensor, EntityChunkManager &entityChunkManager)
	{
		if (isVoxelSensor)
		{
			return;
		}

		entityChunkManager.queueEntityDestroy(projectileInstID, true); // @todo shouldn't need to notify chunk of an arrow dying
	}

	void OnBowProjectileVsEntityContactAdded(const JPH::Body &projectileBody, EntityInstanceID projectileInstID, const JPH::Body &entityBody, EntityInstanceID entityInstID, Game &game)
	{
		EntityChunkManager &entityChunkManager = game.sceneManager.entityChunkManager;
		const EntityInstance &entityInst = entityChunkManager.entities.get(entityInstID);
		if (entityInst.isTransformStatic())
		{
			// Trees, static NPCs, containers, etc..
			return;
		}

		if (!entityInst.canBeKilledInCombat())
		{
			return;
		}

		EntityCombatState &entityCombatState = entityChunkManager.combatStates.get(entityInst.combatStateID);
		if (entityCombatState.isInDeathState())
		{
			return;
		}

		GameState &gameState = game.gameState;
		constexpr bool isFromMeleeWeapon = false;
		gameState.addCombatEntityResult(entityInstID, isFromMeleeWeapon);

		entityChunkManager.queueEntityDestroy(projectileInstID, true); // @todo shouldn't need to notify chunk of an arrow dying
	}
}

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
	SceneManager &sceneManager = this->game.sceneManager;
	EntityChunkManager &entityChunkManager = sceneManager.entityChunkManager;

	const Player &player = this->game.player;
	const JPH::BodyID playerBodyID = player.physicsCharacter->GetBodyID();

	const JPH::Body *playerBody = nullptr;
	const JPH::Body *bowProjectileBody = nullptr;
	EntityInstanceID bowProjectileEntityInstID = -1;
	const JPH::Body *otherBody = nullptr;
	JPH::SubShapeID otherSubShapeID;
	if (playerBodyID == body1.GetID())
	{
		playerBody = &body1;
		otherBody = &body2;
		otherSubShapeID = manifold.mSubShapeID2;
	}
	else if (playerBodyID == body2.GetID())
	{
		playerBody = &body2;
		otherBody = &body1;
		otherSubShapeID = manifold.mSubShapeID1;
	}
	else
	{
		for (const EntityInstanceID entityInstID : entityChunkManager.vfxEntityInstIDs)
		{
			const EntityInstance &entityInst = entityChunkManager.entities.get(entityInstID);
			const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityInst.defID);
			if (entityDef.vfx.type != VfxEntityAnimationType::BowProjectile)
			{
				continue;
			}

			if (entityInst.physicsBodyID == body1.GetID())
			{
				bowProjectileBody = &body1;
				bowProjectileEntityInstID = entityInstID;
				otherBody = &body2;
				otherSubShapeID = manifold.mSubShapeID2;
				break;
			}
			else if (entityInst.physicsBodyID == body2.GetID())
			{
				bowProjectileBody = &body2;
				bowProjectileEntityInstID = entityInstID;
				otherBody = &body1;
				otherSubShapeID = manifold.mSubShapeID1;
				break;
			}
		}
	}

	if (otherBody == nullptr)
	{
		// Don't care about this contact pair.
		return;
	}

	const JPH::BodyID otherBodyID = otherBody->GetID();
	const EntityInstanceID otherBodyEntityInstanceID = entityChunkManager.getEntityFromPhysicsBodyID(otherBodyID);

	const bool isPlayerVsEntityCollision = (playerBody != nullptr) && (otherBodyEntityInstanceID >= 0);
	const bool isPlayerVsVoxelCollision = (playerBody != nullptr) && !isPlayerVsEntityCollision;
	const bool isBowProjectileVsEntityCollision = (bowProjectileBody != nullptr) && (otherBodyEntityInstanceID >= 0);
	const bool isBowProjectileVsVoxelCollision = (bowProjectileBody != nullptr) && !isBowProjectileVsEntityCollision;

	if (isPlayerVsVoxelCollision)
	{
		OnPlayerVsVoxelContactAdded(*playerBody, *otherBody, otherSubShapeID, otherBody->IsSensor(), this->game);
	}
	else if (isPlayerVsEntityCollision)
	{
		/*if (otherBody->IsSensor())
		{
			OnPlayerVsEntitySensorContactAdded(*playerBody, *otherBody, this->game);
		}*/
	}
	else if (isBowProjectileVsVoxelCollision)
	{
		OnBowProjectileVsVoxelContactAdded(*bowProjectileBody, bowProjectileEntityInstID, *otherBody, otherSubShapeID, otherBody->IsSensor(), entityChunkManager);
	}
	else if (isBowProjectileVsEntityCollision)
	{
		OnBowProjectileVsEntityContactAdded(*bowProjectileBody, bowProjectileEntityInstID, *otherBody, otherBodyEntityInstanceID, this->game);
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
