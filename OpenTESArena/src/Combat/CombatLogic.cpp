#include <algorithm>

#include "ArenaCombatUtils.h"
#include "CombatLogic.h"
#include "../Assets/ArenaSoundName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Audio/AudioManager.h"
#include "../Entities/ArenaEntityUtils.h"
#include "../Entities/EntityChunkManager.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../Game/Game.h"
#include "../Interface/GameWorldUiState.h"
#include "../Items/ItemLibrary.h"
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

bool CombatLogic::canSourceTypeDamageDoor(CombatResultSourceType sourceType)
{
	return sourceType == CombatResultSourceType::PlayerMeleeAttack;
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

EntityDefID CombatLogic::getVfxEntityDefID(VfxEntityAnimationType vfxType, int index)
{
	EntityDefinitionKey entityDefKey;
	entityDefKey.initVfx(vfxType, index);

	EntityDefID entityDefID;
	if (!EntityDefinitionLibrary::getInstance().tryGetDefinitionID(entityDefKey, &entityDefID))
	{
		DebugLogErrorFormat("Couldn't get entity definition ID for VFX type %d index %d.", vfxType, index);
		return -1;
	}

	return entityDefID;
}

EntityInitInfo CombatLogic::getBowProjectileEntityInitInfo(WorldDouble3 position, Double3 direction)
{
	const EntityDefID bowProjectileEntityDefID = CombatLogic::getVfxEntityDefID(VfxEntityAnimationType::BowProjectile, 0);
	const EntityDefinition &bowProjectileEntityDef = EntityDefinitionLibrary::getInstance().getDefinition(bowProjectileEntityDefID);
	const EntityAnimationDefinition &bowProjectileAnimDef = bowProjectileEntityDef.animDef;

	EntityInitInfo bowProjectileEntityInitInfo;
	bowProjectileEntityInitInfo.defID = bowProjectileEntityDefID;
	bowProjectileEntityInitInfo.feetPosition = position + (direction * 0.10);
	bowProjectileEntityInitInfo.initialAnimStateIndex = *bowProjectileAnimDef.findStateIndex(EntityAnimationUtils::STATE_IDLE.c_str());
	bowProjectileEntityInitInfo.isSensorCollider = true;
	bowProjectileEntityInitInfo.canBeKilled = false;
	bowProjectileEntityInitInfo.direction = direction;
	return bowProjectileEntityInitInfo;
}

EntityInitInfo CombatLogic::getPhysicalHitVfxEntityInitInfo(const EntityDefinition &hitEntityDef, WorldDouble3 position)
{
	constexpr int FirstBloodIndex = 24; // Based on original game VFX array, blood is indices 24-26.
	EntityDefinitionKey key;
	int bloodIndex = FirstBloodIndex;
	if (hitEntityDef.type == EntityDefinitionType::Enemy)
	{
		if (hitEntityDef.enemy.type == EnemyEntityDefinitionType::Creature)
		{
			const CreatureDefinitionLibrary &creatureDefLibrary = CreatureDefinitionLibrary::getInstance();
			const CreatureDefinition &creatureDef = creatureDefLibrary.getDefinition(hitEntityDef.enemy.creatureDefID);
			bloodIndex = creatureDef.bloodIndex;
		}
	}

	bloodIndex -= FirstBloodIndex;
	key.initVfx(VfxEntityAnimationType::PhysicalHit, bloodIndex);

	const EntityDefinitionLibrary &entityDefLibrary = EntityDefinitionLibrary::getInstance();
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
	initInfo.spawnSfxName = ArenaSoundName::EnemyHit;
	return initInfo;
}

EntityInitInfo CombatLogic::getSpellProjectileEntityInitInfo(int spellIndex, WorldDouble3 position, Double3 direction)
{
	const EntityDefID projectileEntityDefID = CombatLogic::getVfxEntityDefID(VfxEntityAnimationType::SpellProjectile, spellIndex);
	const EntityDefinition &projectileEntityDef = EntityDefinitionLibrary::getInstance().getDefinition(projectileEntityDefID);
	const EntityAnimationDefinition &projectileAnimDef = projectileEntityDef.animDef;

	EntityInitInfo projectileEntityInitInfo;
	projectileEntityInitInfo.defID = projectileEntityDefID;
	projectileEntityInitInfo.feetPosition = position + (direction * 0.10);
	projectileEntityInitInfo.initialAnimStateIndex = *projectileAnimDef.findStateIndex(EntityAnimationUtils::STATE_IDLE.c_str());
	projectileEntityInitInfo.isSensorCollider = true;
	projectileEntityInitInfo.canBeKilled = false;
	projectileEntityInitInfo.direction = direction;
	projectileEntityInitInfo.spellIndex = spellIndex;
	projectileEntityInitInfo.spawnSfxName = ArenaSoundName::SlowBall;
	return projectileEntityInitInfo;
}

EntityInitInfo CombatLogic::getSpellExplosionEntityInitInfo(int spellIndex, WorldDouble3 position)
{
	const EntityDefID explosionEntityDefID = CombatLogic::getVfxEntityDefID(VfxEntityAnimationType::SpellExplosion, spellIndex);
	const EntityDefinition &explosionEntityDef = EntityDefinitionLibrary::getInstance().getDefinition(explosionEntityDefID);
	const EntityAnimationDefinition &explosionAnimDef = explosionEntityDef.animDef;

	double maxAnimWidth, maxAnimHeight;
	EntityUtils::getAnimationMaxDims(explosionAnimDef, &maxAnimWidth, &maxAnimHeight);
	const Double3 halfHeightOffset(0.0, maxAnimHeight * 0.50, 0.0);

	EntityInitInfo entityInitInfo;
	entityInitInfo.defID = explosionEntityDefID;
	entityInitInfo.feetPosition = position - halfHeightOffset;
	entityInitInfo.initialAnimStateIndex = *explosionAnimDef.findStateIndex(EntityAnimationUtils::STATE_IDLE.c_str());
	entityInitInfo.isSensorCollider = true;
	entityInitInfo.canBeKilled = false;
	entityInitInfo.spellIndex = spellIndex;
	entityInitInfo.spawnSfxName = ArenaSoundName::Explode;
	return entityInitInfo;
}

void CombatLogic::onVoxelHitByPlayer(WorldInt3 hitWorldVoxel, CombatResultSourceType sourceType, Game &game)
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

	if (CombatLogic::canSourceTypeDamageDoor(sourceType))
	{
		audioManager.playSoundOneShot(ArenaSoundName::Bash, hitWorldVoxelCenter);

		if (sourceType == CombatResultSourceType::PlayerMeleeAttack)
		{
			const ItemDefinitionID playerEquippedWeaponItemDefID = player.getEquippedWeaponItemDefID();
			const bool isFists = playerEquippedWeaponItemDefID < 0;
			if (isFists)
			{
				player.currentHealth -= ArenaPlayerUtils::getSelfDamageFromDoorBashWithFists(random);
			}
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
}

void CombatLogic::onEntityHitByPlayer(EntityInstanceID hitEntityInstID, CombatResultSourceType sourceType, EntityInstanceID sourceEntityInstID, Game &game)
{
	GameState &gameState = game.gameState;
	Player &player = game.player;
	Random &random = game.random;
	AudioManager &audioManager = game.audioManager;
	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();

	EntityChunkManager &entityChunkManager = game.sceneManager.entityChunkManager;
	const EntityInstance &hitEntityInst = entityChunkManager.entities.get(hitEntityInstID);
	const WorldDouble3 hitEntityPosition = entityChunkManager.positions.get(hitEntityInst.positionID);
	const BoundingBox3D &hitEntityBBox = entityChunkManager.boundingBoxes.get(hitEntityInst.bboxID);
	const WorldDouble3 hitEntityMiddlePosition(hitEntityPosition.x, hitEntityPosition.y + hitEntityBBox.halfHeight, hitEntityPosition.z);

	const bool isPhysicalImpact = (sourceType == CombatResultSourceType::PlayerMeleeAttack) || (sourceType == CombatResultSourceType::PlayerBowProjectile);
	const bool isMagicalImpact = sourceType == CombatResultSourceType::PlayerSpellProjectile;

	const DerivedAttributes playerDerivedAttributes = ArenaPlayerUtils::calculateTotalDerivedBonuses(game.player.primaryAttributes);

	int sourceDamage = 0;
	if (sourceType == CombatResultSourceType::PlayerMeleeAttack)
	{
		const ItemDefinitionID playerEquippedWeaponItemDefID = player.getEquippedWeaponItemDefID();
		if (playerEquippedWeaponItemDefID >= 0)
		{
			const ItemDefinition &playerEquippedWeaponItemDef = ItemLibrary::getInstance().getDefinition(playerEquippedWeaponItemDefID);
			const WeaponItemDefinition &weaponItemDef = playerEquippedWeaponItemDef.weapon;
			sourceDamage = weaponItemDef.damageMin + random.next(weaponItemDef.damageMax - weaponItemDef.damageMin + 1);
		}
		else
		{
			const ItemInstancePredicate equippedGauntletsPredicate = [](const ItemInstance &itemInst)
			{
				if (!itemInst.isEquipped)
				{
					return false;
				}

				const ItemDefinition &itemDef = ItemLibrary::getInstance().getDefinition(itemInst.defID);
				return (itemDef.type == ItemType::Armor) && (itemDef.armor.typeID == ArenaItemUtils::HandsArmorTypeID);
			};

			int extraGauntletsDamage = 0;
			int playerEquippedGauntletsInventorySlotIndex;
			if (player.inventory.findFirstValidSlotIf(equippedGauntletsPredicate, &playerEquippedGauntletsInventorySlotIndex))
			{
				const ItemInstance &playerEquippedGauntletsItemInst = player.inventory.getSlot(playerEquippedGauntletsInventorySlotIndex);
				const ItemDefinition &playerEquippedGauntletsItemDef = ItemLibrary::getInstance().getDefinition(playerEquippedGauntletsItemInst.defID);
				extraGauntletsDamage = playerEquippedGauntletsItemDef.armor.armorClass;
			}

			sourceDamage = (1 + random.next(2)) + extraGauntletsDamage;
		}

		sourceDamage += playerDerivedAttributes.bonusDamage;
	}
	else if (sourceType == CombatResultSourceType::PlayerBowProjectile)
	{
		sourceDamage = 3 + random.next(8); // @todo store damage in projectile entity (needs to include bow damage, which may have been unequipped)
	}
	else if (sourceType == CombatResultSourceType::PlayerSpellProjectile)
	{
		sourceDamage = 10 + random.next(40); // @todo spell effects support
	}

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

	if (canHitEntityBeKilled)
	{
		bool isHitEntityTakingDamage = true;

		if (hitEntityDef.type == EntityDefinitionType::Enemy)
		{
			const int playerHitBonus = playerDerivedAttributes.bonusToHit;
			const int playerLuckBonus = 0; // @todo

			int hitEntityClassID = -1;
			if (hitEntityDef.enemy.type == EnemyEntityDefinitionType::Human)
			{
				const int hitHumanEnemyCharClassDefID = hitEntityDef.enemy.human.charClassID;
				const CharacterClassDefinition &hitEntityCharClassDef = CharacterClassLibrary::getInstance().getDefinition(hitHumanEnemyCharClassDefID);
				hitEntityClassID = hitEntityCharClassDef.originalClassIndex;
			}

			int hitEntityDefenseBonus = 0; // @todo
			int hitEntityLuckBonus = 0; // @todo

			if (isPhysicalImpact)
			{
				isHitEntityTakingDamage = ArenaCombatUtils::isMeleeHitSuccessful(player.level, player.raceID, playerHitBonus, playerLuckBonus, hitEntityCombatState->level, hitEntityClassID, hitEntityDefenseBonus, hitEntityLuckBonus, game.arenaRandom);
			}
			else if (isMagicalImpact)
			{
				isHitEntityTakingDamage = true; // @todo spell effects/resist/willpower
			}
		}

		if (isHitEntityTakingDamage)
		{
			hitEntityCombatState->health = std::max(hitEntityCombatState->health - sourceDamage, 0.0);

			// Arbitrary height where the swing is hitting.
			const double hitVfxHeightBias = std::min(PlayerConstants::TOP_OF_HEAD_HEIGHT * 0.60, hitEntityBBox.halfHeight);

			// Avoid z-fighting with entity.
			const Double2 hitVfxPositionBias = -player.getGroundDirectionXZ() * 0.05;

			const WorldDouble3 hitVfxPosition(
				hitEntityPosition.x + hitVfxPositionBias.x,
				hitEntityPosition.y + hitVfxHeightBias,
				hitEntityPosition.z + hitVfxPositionBias.y);

			EntityInitInfo hitVfxEntityInitInfo;
			if (isPhysicalImpact)
			{
				hitVfxEntityInitInfo = CombatLogic::getPhysicalHitVfxEntityInitInfo(hitEntityDef, hitVfxPosition);
			}
			else if (isMagicalImpact)
			{
				const EntityInstance &sourceEntityInst = entityChunkManager.entities.get(sourceEntityInstID);
				const EntitySpellState &sourceEntitySpellState = entityChunkManager.spellStates.get(sourceEntityInst.spellStateID);
				hitVfxEntityInitInfo = CombatLogic::getSpellExplosionEntityInitInfo(sourceEntitySpellState.spellIndex, hitVfxPosition);
			}

			if (hitVfxEntityInitInfo.defID >= 0)
			{
				gameState.queueEntityInstantiate(hitVfxEntityInitInfo);
			}
		}

		if (hitEntityCombatState->health == 0.0)
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
		}
		else
		{
			if (isPhysicalImpact)
			{
				audioManager.playSoundOneShot(ArenaSoundName::Clank, hitEntityMiddlePosition);
			}
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

void CombatLogic::onPlayerHitByEntity(EntityInstanceID sourceEntityInstID, Game &game)
{
	const EntityChunkManager &entityChunkManager = game.sceneManager.entityChunkManager;
	AudioManager &audioManager = game.audioManager;
	Random &random = game.random;
	ArenaRandom &arenaRandom = game.arenaRandom;
	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();

	const EntityInstance &entityInst = entityChunkManager.entities.get(sourceEntityInstID);
	const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityInst.defID);

	int entityCombatLevel = 0;
	if (entityInst.canBeKilledInCombat())
	{
		const EntityCombatState &entityCombatState = entityChunkManager.combatStates.get(entityInst.combatStateID);
		entityCombatLevel = entityCombatState.level;
	}

	const int entityRaceID = 0; // @todo
	const int entityHitBonus = 0; // @todo
	const int entityLuckBonus = 0; // @todo

	Player &player = game.player;
	const CharacterClassDefinition &playerCharClassDef = CharacterClassLibrary::getInstance().getDefinition(player.charClassDefID);
	const int originalPlayerClassIndex = playerCharClassDef.originalClassIndex;

	const DerivedAttributes playerDerivedAttributes = ArenaPlayerUtils::calculateTotalDerivedBonuses(game.player.primaryAttributes);
	int playerDefenseBonus = playerDerivedAttributes.bonusToDefend;
	int playerLuckBonus = 0; // @todo

	const bool isPlayerHit = ArenaCombatUtils::isMeleeHitSuccessful(entityCombatLevel, entityRaceID, entityHitBonus, entityLuckBonus, player.level, originalPlayerClassIndex, playerDefenseBonus, playerLuckBonus, arenaRandom);
	if (!isPlayerHit)
	{
		audioManager.playSoundOneShot(ArenaSoundName::Clank);
		return;
	}

	int damageAmount = 0;

	DebugAssert(entityDef.type == EntityDefinitionType::Enemy);
	const EnemyEntityDefinition &enemyDef = entityDef.enemy;

	if (enemyDef.type == EnemyEntityDefinitionType::Human)
	{
		const ItemInventory &humanEnemyItemInventory = entityChunkManager.itemInventories.get(entityInst.itemInventoryInstID);

		int humanEnemyEquippedWeaponInventorySlotIndex;
		const bool humanEnemyHasWeaponEquipped = humanEnemyItemInventory.findFirstValidSlotDefIf(
			[](const ItemDefinition &itemDef)
		{
			return itemDef.type == ItemType::Weapon;
		}, &humanEnemyEquippedWeaponInventorySlotIndex);

		if (humanEnemyHasWeaponEquipped)
		{
			const ItemDefinitionID humanEnemyEquippedWeaponItemDefID = humanEnemyItemInventory.getSlot(humanEnemyEquippedWeaponInventorySlotIndex).defID;
			const ItemDefinition &humanEnemyEquippedWeaponItemDef = ItemLibrary::getInstance().getDefinition(humanEnemyEquippedWeaponItemDefID);
			const WeaponItemDefinition &humanEnemyEquippedWeaponDef = humanEnemyEquippedWeaponItemDef.weapon;
			damageAmount = humanEnemyEquippedWeaponDef.damageMin + random.next(humanEnemyEquippedWeaponDef.damageMax - humanEnemyEquippedWeaponDef.damageMin + 1);
		}
		else
		{
			damageAmount = 1 + random.next(5); // Arbitrary fists damage. @todo get original fists damage (also count gauntlets damage, see ExeData /wiki/Combat#damage-modifiers)
		}

		// @todo: Chance for critical strike
	}
	else
	{
		const CreatureDefinition &creatureDef = CreatureDefinitionLibrary::getInstance().getDefinition(enemyDef.creatureDefID);
		damageAmount = creatureDef.minDamage + random.next(creatureDef.maxDamage - creatureDef.minDamage + 1);

		const int originalCreatureIndex = static_cast<int>(enemyDef.creatureDefID);

		// @todo: Pass in real values for the saving throw and resistance effect
		const bool isPlayerPoisonResistEffectActive = false;
		const int playerPoisonSavingThrow = 90;

		int diseaseID;
		double diseaseSecondsRemaining;
		double paralysisSecondsRemaining;
		ArenaEntityUtils::paralysisOrDiseaseOnHit(originalCreatureIndex, player.raceID, originalPlayerClassIndex, isPlayerPoisonResistEffectActive,
			playerPoisonSavingThrow, arenaRandom, exeData, &diseaseID, &diseaseSecondsRemaining, &paralysisSecondsRemaining);

		if (diseaseID >= 0)
		{
			player.effectsState.applyDisease(diseaseID, diseaseSecondsRemaining);
		}

		if (paralysisSecondsRemaining > 0.0)
		{
			player.effectsState.applyParalysis(paralysisSecondsRemaining);
		}
	}

	if (game.options.getMisc_ImmuneToDamage())
	{
		damageAmount = 0;
	}

	if (damageAmount > 0)
	{
		player.currentHealth = std::max(player.currentHealth - damageAmount, 0.0);
		GameWorldUI::showPlayerHurt();
	}

	audioManager.playSoundOneShot(ArenaSoundName::PlayerHit);
}
