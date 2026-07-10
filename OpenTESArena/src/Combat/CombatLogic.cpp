#include <algorithm>

#include "CombatLogic.h"
#include "../Assets/ArenaSoundName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Audio/AudioManager.h"
#include "../Entities/ArenaEntityUtils.h"
#include "../Entities/EntityChunkManager.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../Game/Game.h"
#include "../Math/BoundingBox.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../Voxels/VoxelChunkManager.h"
#include "../World/MapLogic.h"

CombatHitSearchResult::CombatHitSearchResult()
{
	this->voxelCount = 0;
	std::fill(std::begin(this->entities), std::end(this->entities), -1);
	this->entityCount = 0;
}

Span<const WorldInt3> CombatHitSearchResult::getVoxels() const
{
	return Span<const WorldInt3>(this->voxels, this->voxelCount);
}

Span<const EntityInstanceID> CombatHitSearchResult::getEntities() const
{
	return Span<const EntityInstanceID>(this->entities, this->entityCount);
}

void CombatLogic::getHitSearchResult(const WorldDouble3 &searchPoint, double searchRadius, double ceilingScale,
	const VoxelChunkManager &voxelChunkManager, const EntityChunkManager &entityChunkManager, CombatHitSearchResult *outHitSearchResult)
{
	const double searchDim = searchRadius * 2.0;
	BoundingBox3D searchBBox;
	searchBBox.init(searchPoint, searchDim, searchDim, searchDim);

	const WorldDouble3 searchMinWorldPoint = searchBBox.min;
	const WorldDouble3 searchMaxWorldPoint = searchBBox.max;
	const WorldInt3 searchMinWorldVoxel = VoxelUtils::pointToVoxel(searchMinWorldPoint, ceilingScale);
	const WorldInt3 searchMaxWorldVoxel = VoxelUtils::pointToVoxel(searchMaxWorldPoint, ceilingScale);

	for (WEInt z = searchMinWorldVoxel.z; z <= searchMaxWorldVoxel.z; z++)
	{
		for (int y = searchMinWorldVoxel.y; y <= searchMaxWorldVoxel.y; y++)
		{
			for (SNInt x = searchMinWorldVoxel.x; x <= searchMaxWorldVoxel.x; x++)
			{
				// Only want voxels in gameplay.
				const WorldInt3 searchWorldVoxel(x, y, z);
				const CoordInt3 searchVoxelCoord = VoxelUtils::worldVoxelToCoord(searchWorldVoxel);
				const VoxelInt3 searchVoxel = searchVoxelCoord.voxel;
				const VoxelChunk *voxelChunk = voxelChunkManager.findChunkAtPosition(searchVoxelCoord.chunk);
				if ((voxelChunk == nullptr) || !voxelChunk->isValidVoxel(searchVoxel.x, searchVoxel.y, searchVoxel.z))
				{
					continue;
				}

				if (outHitSearchResult->voxelCount == CombatHitSearchResult::MAX_HIT_COUNT)
				{
					break;
				}

				outHitSearchResult->voxels[outHitSearchResult->voxelCount] = WorldInt3(x, y, z);
				outHitSearchResult->voxelCount++;
			}
		}
	}

	const ChunkInt2 searchMinChunk = VoxelUtils::worldVoxelToChunk(searchMinWorldVoxel);
	const ChunkInt2 searchMaxChunk = VoxelUtils::worldVoxelToChunk(searchMaxWorldVoxel);

	for (WEInt chunkZ = searchMinChunk.y; chunkZ <= searchMaxChunk.y; chunkZ++)
	{
		for (SNInt chunkX = searchMinChunk.x; chunkX <= searchMaxChunk.x; chunkX++)
		{
			const ChunkInt2 searchChunkPos(chunkX, chunkZ);
			const EntityChunk *entityChunk = entityChunkManager.findChunkAtPosition(searchChunkPos);
			if (entityChunk != nullptr)
			{
				for (const EntityInstanceID entityInstID : entityChunk->entityIDs)
				{
					const EntityInstance &entityInst = entityChunkManager.entities.get(entityInstID);
					if (!entityInst.canAcceptCombatHits())
					{
						continue;
					}

					const WorldDouble3 entityPosition = entityChunkManager.positions.get(entityInst.positionID);
					const BoundingBox3D &entityBBox = entityChunkManager.boundingBoxes.get(entityInst.bboxID);
					const WorldDouble3 entityWorldBBoxMin = entityPosition + entityBBox.min;
					const WorldDouble3 entityWorldBBoxMax = entityPosition + entityBBox.max;
					BoundingBox3D entityWorldBBox;
					entityWorldBBox.init(entityWorldBBoxMin, entityWorldBBoxMax);

					if (searchBBox.intersects(entityWorldBBox))
					{
						if (outHitSearchResult->entityCount == CombatHitSearchResult::MAX_HIT_COUNT)
						{
							break;
						}

						outHitSearchResult->entities[outHitSearchResult->entityCount] = entityInstID;
						outHitSearchResult->entityCount++;
					}
				}
			}
		}
	}
}

void CombatLogic::spawnBowProjectile(WorldDouble3 position, Double2 direction, EntityChunkManager &entityChunkManager,
	Random &random, JPH::PhysicsSystem &physicsSystem, Renderer &renderer)
{
	EntityDefinitionKey bowProjectileEntityDefKey;
	bowProjectileEntityDefKey.initVfx(VfxEntityAnimationType::BowProjectile, 0);

	const EntityDefinitionLibrary &entityDefLibrary = EntityDefinitionLibrary::getInstance();
	EntityDefID bowProjectileEntityDefID;
	if (!entityDefLibrary.tryGetDefinitionID(bowProjectileEntityDefKey, &bowProjectileEntityDefID))
	{
		DebugCrash("Couldn't get bow projectile entity definition ID.");
	}

	const EntityDefinition &bowProjectileEntityDef = entityDefLibrary.getDefinition(bowProjectileEntityDefID);
	const EntityAnimationDefinition &bowProjectileAnimDef = bowProjectileEntityDef.animDef;

	EntityInitInfo bowProjectileEntityInitInfo;
	bowProjectileEntityInitInfo.defID = bowProjectileEntityDefID;
	bowProjectileEntityInitInfo.feetPosition = position + (Double3(direction.x, 0.0, direction.y) * 0.10);
	bowProjectileEntityInitInfo.initialAnimStateIndex = *bowProjectileAnimDef.findStateIndex(EntityAnimationUtils::STATE_IDLE.c_str());
	bowProjectileEntityInitInfo.isSensorCollider = true;
	bowProjectileEntityInitInfo.canBeKilled = false;
	bowProjectileEntityInitInfo.direction = direction;
	entityChunkManager.createEntity(bowProjectileEntityInitInfo, random, physicsSystem, renderer);
}

void CombatLogic::spawnHitVfx(const EntityDefinition &hitEntityDef, const WorldDouble3 &position, EntityChunkManager &entityChunkManager,
	Random &random, JPH::PhysicsSystem &physicsSystem, Renderer &renderer)
{
	const EntityDefinitionLibrary &entityDefLibrary = EntityDefinitionLibrary::getInstance();

	constexpr int FirstBloodIndex = 24; // Based on original game VFX array, blood is indices 24-26.
	EntityDefinitionKey key;
	int bloodIndex = FirstBloodIndex;
	if (hitEntityDef.type == EntityDefinitionType::Enemy && hitEntityDef.enemy.type == EnemyEntityDefinitionType::Creature)
	{
		const CreatureDefinitionLibrary &creatureDefLibrary = CreatureDefinitionLibrary::getInstance();
		const CreatureDefinition &creatureDef = creatureDefLibrary.getDefinition(hitEntityDef.enemy.creatureDefID);
		bloodIndex = creatureDef.bloodIndex;
	}

	bloodIndex -= FirstBloodIndex;
	key.initVfx(VfxEntityAnimationType::MeleeStrike, bloodIndex);

	EntityDefID hitEntityVfxEntityDefID;
	if (!entityDefLibrary.tryGetDefinitionID(key, &hitEntityVfxEntityDefID))
	{
		DebugCrash("Couldn't get hit entity VFX definition ID from library.");
	}

	EntityInitInfo initInfo;
	initInfo.defID = hitEntityVfxEntityDefID;
	initInfo.feetPosition = position;
	initInfo.initialAnimStateIndex = 0; // Assuming VFX only have one state.
	initInfo.isSensorCollider = true;
	entityChunkManager.createEntity(initInfo, random, physicsSystem, renderer);
}

void CombatLogic::onVoxelHitByPlayer(WorldInt3 hitWorldVoxel, bool anyWeaponEquipped, Game &game)
{
	const CoordInt3 hitVoxelCoord = VoxelUtils::worldVoxelToCoord(hitWorldVoxel);
	const VoxelInt3 hitVoxel = hitVoxelCoord.voxel;

	VoxelChunkManager &voxelChunkManager = game.sceneManager.voxelChunkManager;
	VoxelChunk &hitVoxelChunk = voxelChunkManager.getChunkAtPosition(hitVoxelCoord.chunk);

	VoxelDoorDefID doorDefID;
	if (!hitVoxelChunk.tryGetDoorDefID(hitVoxel.x, hitVoxel.y, hitVoxel.z, &doorDefID))
	{
		return;
	}

	// Can't hit if already open.
	int doorAnimInstIndex;
	if (hitVoxelChunk.tryGetDoorAnimInstIndex(hitVoxel.x, hitVoxel.y, hitVoxel.z, &doorAnimInstIndex))
	{
		return;
	}

	// Can only hit if not previously unlocked.
	int triggerInstIndex;
	const bool isVoxelPreviouslyUnlocked = hitVoxelChunk.tryGetTriggerInstIndex(hitVoxel.x, hitVoxel.y, hitVoxel.z, &triggerInstIndex);
	if (isVoxelPreviouslyUnlocked)
	{
		return;
	}

	const GameState &gameState = game.gameState;
	Player &player = game.player;
	Random &random = game.random;
	const double ceilingScale = gameState.getActiveCeilingScale();

	VoxelLockDefID lockDefID;
	if (!hitVoxelChunk.tryGetLockDefID(hitVoxel.x, hitVoxel.y, hitVoxel.z, &lockDefID))
	{
		return;
	}

	const LockDefinition &lockDef = hitVoxelChunk.lockDefs[lockDefID];
	const bool isDoorBashable = lockDef.lockLevel >= 0;
	if (!isDoorBashable)
	{
		return;
	}

	const WorldDouble3 hitWorldVoxelCenter = VoxelUtils::getVoxelCenter(hitWorldVoxel, ceilingScale);

	AudioManager &audioManager = game.audioManager;
	audioManager.playSoundOneShot(ArenaSoundName::Bash, hitWorldVoxelCenter);

	if (!anyWeaponEquipped)
	{
		player.currentHealth -= ArenaPlayerUtils::getSelfDamageFromDoorBashWithFists(random);
	}

	const int doorBashDamage = ArenaPlayerUtils::DoorBashMinDamageRequired; // @todo: Calculate damage
	if (!ArenaPlayerUtils::isDoorBashSuccessful(doorBashDamage, lockDef.lockLevel, player.primaryAttributes, random))
	{
		return;
	}

	constexpr bool isApplyingDoorKeyToLock = false;
	constexpr int doorKeyID = -1;
	constexpr bool isLockpickingSuccessful = false;
	constexpr bool isWeaponBashing = true;
	MapLogic::handleDoorOpen(game, hitVoxelChunk, hitVoxel, ceilingScale, isApplyingDoorKeyToLock, doorKeyID, isLockpickingSuccessful, isWeaponBashing);
}

void CombatLogic::onEntityHitByPlayer(EntityInstanceID hitEntityInstID, Game &game)
{
	EntityChunkManager &entityChunkManager = game.sceneManager.entityChunkManager;
	const EntityInstance &hitEntityInst = entityChunkManager.entities.get(hitEntityInstID);
	const WorldDouble3 hitEntityPosition = entityChunkManager.positions.get(hitEntityInst.positionID);
	const BoundingBox3D &hitEntityBBox = entityChunkManager.boundingBoxes.get(hitEntityInst.bboxID);
	const WorldDouble3 hitEntityMiddlePosition(hitEntityPosition.x, hitEntityPosition.y + hitEntityBBox.halfHeight, hitEntityPosition.z);

	const EntityDefinition &hitEntityDef = entityChunkManager.getEntityDef(hitEntityInst.defID);
	const EntityAnimationDefinition &hitEntityAnimDef = hitEntityDef.animDef;
	EntityAnimationInstance &hitEntityAnimInst = entityChunkManager.animInsts.get(hitEntityInst.animInstID);

	EntityCombatState *hitEntityCombatState = nullptr;
	bool canHitEntityBeKilled = false;
	if (hitEntityInst.canBeKilledInCombat())
	{
		hitEntityCombatState = &entityChunkManager.combatStates.get(hitEntityInst.combatStateID);
		canHitEntityBeKilled = !hitEntityCombatState->isInDeathState();
	}

	EntityLockState *hitEntityLockState = nullptr;
	bool canHitEntityLockBeBroken = false;
	if (hitEntityInst.canBeLocked())
	{
		hitEntityLockState = &entityChunkManager.lockStates.get(hitEntityInst.lockStateID);
		canHitEntityLockBeBroken = hitEntityLockState->isLocked;
	}

	GameState &gameState = game.gameState;
	Player &player = game.player;
	Random &random = game.random;
	AudioManager &audioManager = game.audioManager;
	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();

	if (canHitEntityBeKilled)
	{
		// Simulate weapon swing against them.
		const bool canHitEntityResistDamage = hitEntityDef.type == EntityDefinitionType::Enemy; // @todo give citizens only 1 hp
		const bool isHitEntityHpAtZero = !canHitEntityResistDamage || random.nextBool(); // @todo actual hp dmg calculation

		if (isHitEntityHpAtZero)
		{
			hitEntityCombatState->isDying = true;

			const EntityBehaviorState &hitEntityBehaviorState = entityChunkManager.behaviorStates.get(hitEntityInst.behaviorStateID);
			if (hitEntityBehaviorState.isCitizen())
			{
				GameWorldUiController::onCitizenKilled(game);
				gameState.queueCityGuardEncounter(game);
			}
			else if (hitEntityBehaviorState.isCreature())
			{
				DebugAssert(hitEntityDef.enemy.type == EnemyEntityDefinitionType::Creature);
				const CreatureDefinition &hitEntityCreatureDef = CreatureDefinitionLibrary::getInstance().getDefinition(hitEntityDef.enemy.creatureDefID);
				const int creatureCalculatedExperience = hitEntityCreatureDef.baseExp + (hitEntityCreatureDef.maxHP * hitEntityCreatureDef.expMultiplier);
				player.experience += creatureCalculatedExperience;
				DebugLogFormat("%s gave %d XP.", hitEntityCreatureDef.name, creatureCalculatedExperience);
			}
			else if (hitEntityBehaviorState.isHumanEnemy())
			{
				const int hitHumanEnemyLevel = hitEntityCombatState->level;
				DebugAssert(hitEntityDef.enemy.type == EnemyEntityDefinitionType::Human);
				const int hitHumanEnemyCharClassDefID = hitEntityDef.enemy.human.charClassID;
				const CharacterClassDefinition &hitEntityCharClassDef = CharacterClassLibrary::getInstance().getDefinition(hitHumanEnemyCharClassDefID);
				const int humanEnemyCalculatedExperience = ArenaEntityUtils::getHumanEnemyExperience(hitHumanEnemyLevel, hitHumanEnemyCharClassDefID, exeData);
				player.experience += humanEnemyCalculatedExperience;
				DebugLogFormat("%s gave %d XP.", hitEntityCharClassDef.name, humanEnemyCalculatedExperience);
			}

			// Arbitrary height where the swing is hitting.
			const double hitVfxHeightBias = std::min(PlayerConstants::TOP_OF_HEAD_HEIGHT * 0.60, hitEntityBBox.halfHeight);

			// Avoid z-fighting with entity.
			const Double2 hitVfxPositionBias = -player.getGroundDirectionXZ() * Constants::Epsilon;

			const WorldDouble3 hitVfxPosition(
				hitEntityPosition.x + hitVfxPositionBias.x,
				hitEntityPosition.y + hitVfxHeightBias,
				hitEntityPosition.z + hitVfxPositionBias.y);

			CombatLogic::spawnHitVfx(hitEntityDef, hitVfxPosition, entityChunkManager, random, game.physicsSystem, game.renderer);

			audioManager.playSoundOneShot(ArenaSoundName::EnemyHit, hitEntityMiddlePosition);
		}
		else
		{
			audioManager.playSoundOneShot(ArenaSoundName::Clank, hitEntityMiddlePosition);
		}
	}
	else if (canHitEntityLockBeBroken)
	{
		const bool isLockBashSuccessful = random.nextBool(); // @todo actual lock bash calculation

		if (isLockBashSuccessful)
		{
			hitEntityLockState->isLocked = false;

			const std::optional<int> unlockedAnimDefStateIndex = hitEntityAnimDef.findStateIndex(EntityAnimationUtils::STATE_UNLOCKED.c_str());
			DebugAssert(unlockedAnimDefStateIndex.has_value());
			hitEntityAnimInst.setStateIndex(*unlockedAnimDefStateIndex);
		}

		audioManager.playSoundOneShot(ArenaSoundName::Bash, hitEntityMiddlePosition);
	}
}
