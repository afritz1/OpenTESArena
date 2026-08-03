#include <mutex>
#include <thread>

#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/Body.h"

#include "PhysicsContactListener.h"
#include "../Assets/ArenaSoundName.h"
#include "../Combat/CombatLogic.h"
#include "../Game/Game.h"
#include "../Voxels/VoxelUtils.h"
#include "../World/MapLogic.h"
#include "../World/MapType.h"

#include "components/debug/Debug.h"

namespace
{
	// Required in contact listeners when Jolt is multi-threaded.
	// - @todo ideally there would be thread-safe queues instead of locking per contact
	std::mutex PlayerVsVoxelMutex;
	std::mutex ProjectileVsVoxelMutex;
	std::mutex ProjectileVsEntityMutex;

	// Converts the physics collider's center of mass to a voxel coordinate. The collider might be combined which
	// makes this unsuitable for anything but single-voxel sensor colliders currently.
	WorldInt3 GetVoxelBodyWorldVoxel(const JPH::Body &body, JPH::SubShapeID subShapeID, double ceilingScale)
	{
		const JPH::Vec3 otherBodyScale = JPH::Vec3::sReplicate(1.0f);

		// Determine which subshape was hit since it's in a compound shape.
		JPH::SubShapeID remainderSubShapeID;
		const JPH::TransformedShape otherSubShapeTransformed = body.GetShape()->GetSubShapeTransformedShape(subShapeID, body.GetCenterOfMassPosition(), body.GetRotation(), otherBodyScale, remainderSubShapeID);

		const JPH::RVec3 otherSubShapePosition = otherSubShapeTransformed.mShapePositionCOM;
		const WorldDouble3 otherSubShapeWorldPosition(
			static_cast<SNInt>(otherSubShapePosition.GetX()),
			static_cast<int>(otherSubShapePosition.GetY()),
			static_cast<WEInt>(otherSubShapePosition.GetZ()));
		return VoxelUtils::pointToVoxel(otherSubShapeWorldPosition, ceilingScale);
	}

	void OnPlayerVsVoxelContactAdded(const JPH::Body &playerBody, const JPH::Body &voxelBody, JPH::SubShapeID voxelSubShapeID, bool isVoxelSensor, Game &game)
	{
		if (!isVoxelSensor)
		{
			return;
		}
		
		std::lock_guard<std::mutex> lockGuard(PlayerVsVoxelMutex);

		const VoxelChunkManager &voxelChunkManager = game.sceneManager.voxelChunkManager;
		JPH::PhysicsSystem &physicsSystem = game.physicsSystem;
		GameState &gameState = game.gameState;
		const double ceilingScale = gameState.getActiveCeilingScale();
		const WorldInt3 otherSubShapeWorldVoxel = GetVoxelBodyWorldVoxel(voxelBody, voxelSubShapeID, ceilingScale);
		const CoordInt3 otherSubShapeVoxelCoord = VoxelUtils::worldVoxelToCoord(otherSubShapeWorldVoxel);
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

	void OnProjectileVsVoxelContactAdded(const JPH::Body &projectileBody, EntityInstanceID projectileInstID, VfxEntityAnimationType projectileVfxAnimType,
		const JPH::Body &voxelBody, JPH::SubShapeID voxelSubShapeID, bool isVoxelSensor, Game &game)
	{
		if (isVoxelSensor)
		{
			return;
		}

		// Note: this handler doesn't support getting the exact voxel that was hit due to collider combining.
		std::lock_guard<std::mutex> lockGuard(ProjectileVsVoxelMutex);

		if (projectileVfxAnimType == VfxEntityAnimationType::SpellProjectile)
		{
			const JPH::RVec3 projectilePhysicsPosition = projectileBody.GetPosition();
			const WorldDouble3 projectileWorldPosition(
				static_cast<SNDouble>(projectilePhysicsPosition.GetX()),
				static_cast<double>(projectilePhysicsPosition.GetY()),
				static_cast<WEDouble>(projectilePhysicsPosition.GetZ()));

			GameState &gameState = game.gameState;
			Random &random = game.random;

			const int spellExplosionIndex = random.next(CombatLogic::SPELL_PROJECTILE_TYPE_COUNT); // @todo provide as parameter
			const EntityDefID explosionEntityDefID = CombatLogic::getSpellExplosionEntityDefID(spellExplosionIndex);
			const EntityDefinition &explosionEntityDef = EntityDefinitionLibrary::getInstance().getDefinition(explosionEntityDefID);
			const EntityAnimationDefinition &explosionAnimDef = explosionEntityDef.animDef;

			EntityInitInfo explosionEntityInitInfo;
			explosionEntityInitInfo.defID = explosionEntityDefID;
			explosionEntityInitInfo.feetPosition = projectileWorldPosition;
			explosionEntityInitInfo.initialAnimStateIndex = *explosionAnimDef.findStateIndex(EntityAnimationUtils::STATE_IDLE.c_str());
			explosionEntityInitInfo.isSensorCollider = true;
			explosionEntityInitInfo.canBeKilled = false;
			gameState.queueEntityInstantiate(explosionEntityInitInfo);

			AudioManager &audioManager = game.audioManager;
			audioManager.playSoundOneShot(ArenaSoundName::Explode, projectileWorldPosition);
		}

		EntityChunkManager &entityChunkManager = game.sceneManager.entityChunkManager;
		entityChunkManager.queueEntityDestroy(projectileInstID, true); // @todo shouldn't need to notify chunk of a projectile dying
	}

	void OnProjectileVsEntityContactAdded(const JPH::Body &projectileBody, EntityInstanceID projectileInstID, VfxEntityAnimationType projectileVfxAnimType,
		const JPH::Body &entityBody, EntityInstanceID entityInstID, Game &game)
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
		
		CombatResultSourceType sourceType;
		switch (projectileVfxAnimType)
		{
		case VfxEntityAnimationType::BowProjectile:
			sourceType = CombatResultSourceType::PlayerBowProjectile;
			break;
		case VfxEntityAnimationType::SpellProjectile:
			sourceType = CombatResultSourceType::PlayerSpellProjectile;
			break;
		default:
			DebugNotImplementedMsg(std::to_string(static_cast<int>(projectileVfxAnimType)));
			break;
		}

		std::lock_guard<std::mutex> lockGuard(ProjectileVsEntityMutex);

		gameState.addCombatEntityResult(entityInstID, sourceType);

		entityChunkManager.queueEntityDestroy(projectileInstID, true); // @todo shouldn't need to notify chunk of a projectile dying
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

	const JPH::Body *projectileBody = nullptr;
	EntityInstanceID projectileEntityInstID = -1;
	VfxEntityAnimationType projectileVfxAnimType = static_cast<VfxEntityAnimationType>(-1);

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
			const VfxEntityAnimationType entityVfxAnimType = entityDef.vfx.type;
			const bool isProjectile = (entityVfxAnimType == VfxEntityAnimationType::BowProjectile) || (entityVfxAnimType == VfxEntityAnimationType::SpellProjectile);
			if (!isProjectile)
			{
				continue;
			}

			if (entityInst.physicsBodyID == body1.GetID())
			{
				projectileBody = &body1;
				projectileEntityInstID = entityInstID;
				projectileVfxAnimType = entityVfxAnimType;
				otherBody = &body2;
				otherSubShapeID = manifold.mSubShapeID2;
				break;
			}
			else if (entityInst.physicsBodyID == body2.GetID())
			{
				projectileBody = &body2;
				projectileEntityInstID = entityInstID;
				projectileVfxAnimType = entityVfxAnimType;
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
	const bool isProjectileVsEntityCollision = (projectileBody != nullptr) && (otherBodyEntityInstanceID >= 0);
	const bool isProjectileVsVoxelCollision = (projectileBody != nullptr) && !isProjectileVsEntityCollision;

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
	else if (isProjectileVsVoxelCollision)
	{
		OnProjectileVsVoxelContactAdded(*projectileBody, projectileEntityInstID, projectileVfxAnimType, *otherBody, otherSubShapeID, otherBody->IsSensor(), this->game);
	}
	else if (isProjectileVsEntityCollision)
	{
		OnProjectileVsEntityContactAdded(*projectileBody, projectileEntityInstID, projectileVfxAnimType, *otherBody, otherBodyEntityInstanceID, this->game);
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
