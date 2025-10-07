#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"

#include "ArenaAnimUtils.h"
#include "ArenaCitizenUtils.h"
#include "ArenaEntityUtils.h"
#include "EntityChunkManager.h"
#include "EntityDefinitionLibrary.h"
#include "EntityObservedResult.h"
#include "../Assets/ArenaSoundName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/MIFUtils.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Assets/TextureManager.h"
#include "../Audio/AudioManager.h"
#include "../Collision/PhysicsLayer.h"
#include "../Items/ItemLibrary.h"
#include "../Math/Constants.h"
#include "../Math/RandomUtils.h"
#include "../Math/Random.h"
#include "../Player/Player.h"
#include "../Player/WeaponAnimationLibrary.h"
#include "../Rendering/Renderer.h"
#include "../Voxels/VoxelChunk.h"
#include "../Voxels/VoxelChunkManager.h"
#include "../World/CardinalDirection.h"
#include "../World/ChunkUtils.h"
#include "../World/LevelDefinition.h"
#include "../World/LevelInfoDefinition.h"
#include "../World/MapDefinition.h"
#include "../World/MapType.h"

#include "components/utilities/Enum.h"
#include "components/utilities/String.h"

namespace
{
	bool TryCreatePhysicsCollider(const WorldDouble3 &feetPosition, double colliderHeight, bool isSensor, JPH::PhysicsSystem &physicsSystem, JPH::BodyID *outBodyID)
	{
		JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();

		const double capsuleHalfTotalHeight = colliderHeight * 0.50;
		const double capsuleRadius = std::min(capsuleHalfTotalHeight, 0.20);
		const double capsuleCylinderHeight = std::max(colliderHeight - (capsuleRadius * 2.0), 0.0);
		const double capsuleCylinderHalfHeight = capsuleCylinderHeight * 0.50;
		DebugAssert(capsuleCylinderHalfHeight >= 0.0);

		JPH::CapsuleShapeSettings capsuleShapeSettings(static_cast<float>(capsuleCylinderHalfHeight), static_cast<float>(capsuleRadius));
		capsuleShapeSettings.SetEmbedded(); // Marked embedded to prevent it from being freed when its ref count reaches 0.
		// @todo: make sure this ^ isn't leaking when we remove/destroy the body

		JPH::ShapeSettings::ShapeResult capsuleShapeResult = capsuleShapeSettings.Create();
		if (capsuleShapeResult.HasError())
		{
			DebugLogError("Couldn't create Jolt capsule shape settings: " + std::string(capsuleShapeResult.GetError().c_str()));
			return false;
		}

		JPH::ShapeRefC capsuleShape = capsuleShapeResult.Get();
		const JPH::RVec3 capsuleJoltPos(
			static_cast<float>(feetPosition.x),
			static_cast<float>(feetPosition.y + capsuleHalfTotalHeight),
			static_cast<float>(feetPosition.z));
		const JPH::Quat capsuleJoltQuat = JPH::Quat::sRotation(JPH::Vec3Arg::sAxisY(), 0.0f);
		const JPH::ObjectLayer capsuleObjectLayer = isSensor ? PhysicsLayers::SENSOR : PhysicsLayers::MOVING;
		JPH::BodyCreationSettings capsuleSettings(capsuleShape, capsuleJoltPos, capsuleJoltQuat, JPH::EMotionType::Kinematic, capsuleObjectLayer);
		capsuleSettings.mIsSensor = isSensor;

		const JPH::Body *capsule = bodyInterface.CreateBody(capsuleSettings);
		if (capsule == nullptr)
		{
			const uint32_t totalBodyCount = physicsSystem.GetNumBodies();
			DebugLogError("Couldn't create Jolt body for entity (total: " + std::to_string(totalBodyCount) + ").");
			return false;
		}

		const JPH::BodyID &capsuleBodyID = capsule->GetID();
		bodyInterface.AddBody(capsuleBodyID, JPH::EActivation::Activate); // @todo: inefficient to add one at a time
		*outBodyID = capsuleBodyID;
		return true;
	}

	Buffer<ScopedObjectTextureRef> MakeAnimTextureRefs(const EntityAnimationDefinition &animDef, TextureManager &textureManager, Renderer &renderer)
	{
		const int keyframeCount = animDef.keyframeCount;
		Buffer<ScopedObjectTextureRef> animTextureRefs(keyframeCount);

		for (int i = 0; i < keyframeCount; i++)
		{
			const EntityAnimationDefinitionKeyframe &keyframe = animDef.keyframes[i];
			const TextureAsset &textureAsset = keyframe.textureAsset;
			const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAsset);
			if (!textureBuilderID.has_value())
			{
				DebugLogWarningFormat("Couldn't load entity anim texture \"%s\".", textureAsset.filename.c_str());
				continue;
			}

			const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
			const ObjectTextureID textureID = renderer.createObjectTexture(textureBuilder.width, textureBuilder.height, textureBuilder.bytesPerTexel);
			if (textureID < 0)
			{
				DebugLogWarningFormat("Couldn't create entity anim texture \"%s\".", textureAsset.filename.c_str());
				continue;
			}

			if (!renderer.populateObjectTexture(textureID, textureBuilder.bytes))
			{
				DebugLogWarningFormat("Couldn't populate entity anim texture \"%s\".", textureAsset.filename.c_str());
			}

			ScopedObjectTextureRef textureRef(textureID, renderer);
			animTextureRefs.set(i, std::move(textureRef));
		}

		return animTextureRefs;
	}

	double GetElevatedPlatformHeight(const VoxelShapeDefinition &voxelShapeDef, double ceilingScale)
	{
		if (!voxelShapeDef.isElevatedPlatform)
		{
			return 0.0;
		}

		DebugAssert(voxelShapeDef.type == VoxelShapeType::Box);
		const double shapeYPos = voxelShapeDef.box.yOffset + voxelShapeDef.box.height;
		return MeshUtils::getScaledVertexY(shapeYPos, voxelShapeDef.scaleType, ceilingScale);
	}
}

EntityCitizenName::EntityCitizenName(const char *name)
{
	std::snprintf(this->name, std::size(this->name), "%s", name);
}

EntityCitizenName::EntityCitizenName()
{
	std::fill(std::begin(this->name), std::end(this->name), '\0');
}

EntityInitInfo::EntityInitInfo()
{
	this->defID = -1;
	this->initialAnimStateIndex = -1;
	this->isSensorCollider = false;
	this->canBeKilled = false;
	this->hasInventory = false;
	this->hasCreatureSound = false;
}

EntityCombatState::EntityCombatState()
{
	this->isDying = false;
	this->isDead = false;
	this->hasBeenLootedBefore = false;
}

bool EntityCombatState::isInDeathState() const
{
	return this->isDying || this->isDead;
}

EntityLockState::EntityLockState()
{
	this->isLocked = false;
}

EntityTransferResult::EntityTransferResult()
{
	this->id = -1;
}

const EntityDefinition &EntityChunkManager::getEntityDef(EntityDefID defID) const
{
	const EntityDefinitionLibrary &defLibrary = EntityDefinitionLibrary::getInstance();
	const auto iter = this->entityDefs.find(defID);
	if (iter != this->entityDefs.end())
	{
		return iter->second;
	}
	else
	{
		return defLibrary.getDefinition(defID);
	}
}

EntityDefID EntityChunkManager::addEntityDef(EntityDefinition &&def, const EntityDefinitionLibrary &defLibrary)
{
	const int libraryDefCount = defLibrary.getDefinitionCount();
	const EntityDefID defID = static_cast<EntityDefID>(libraryDefCount + this->entityDefs.size());
	this->entityDefs.emplace(defID, std::move(def));
	return defID;
}

EntityDefID EntityChunkManager::getOrAddEntityDefID(const EntityDefinition &def, const EntityDefinitionLibrary &defLibrary)
{
	for (const auto &pair : this->entityDefs)
	{
		const EntityDefID currentDefID = pair.first;
		const EntityDefinition &currentDef = pair.second;
		if (currentDef == def) // There doesn't seem to be a better way than value comparisons.
		{
			return currentDefID;
		}
	}

	return this->addEntityDef(EntityDefinition(def), defLibrary);
}

int EntityChunkManager::findAvailableTransformHeapIndex() const
{
	int heapIndex = -1;

	for (int i = 0; i < static_cast<int>(this->transformHeaps.size()); i++)
	{
		const RenderTransformHeap &heap = this->transformHeaps[i];
		if (heap.pool.canAlloc())
		{
			heapIndex = i;
			break;
		}
	}

	return heapIndex;
}

void EntityChunkManager::initializeEntity(EntityInstance &entityInst, EntityInstanceID instID, const EntityDefinition &entityDef,
	const EntityAnimationDefinition &animDef, const EntityInitInfo &initInfo, Random &random, JPH::PhysicsSystem &physicsSystem, Renderer &renderer)
{
	const EntityPositionID positionID = this->positions.alloc();
	if (positionID < 0)
	{
		DebugLogError("Couldn't allocate EntityPositionID.");
	}

	const EntityBoundingBoxID bboxID = this->boundingBoxes.alloc();
	if (bboxID < 0)
	{
		DebugLogError("Couldn't allocate EntityBoundingBoxID.");
	}

	int transformHeapIndex = this->findAvailableTransformHeapIndex();
	if (transformHeapIndex < 0)
	{
		transformHeapIndex = static_cast<int>(this->transformHeaps.size());

		RenderTransformHeap &newTransformHeap = this->transformHeaps.emplace_back(RenderTransformHeap());
		newTransformHeap.uniformBufferID = renderer.createUniformBufferMatrix4s(RenderTransformHeap::MAX_TRANSFORMS);
		if (newTransformHeap.uniformBufferID < 0)
		{
			DebugLogError("Couldn't create uniform buffer for entity transforms.");
		}
	}

	RenderTransformHeap &transformHeap = this->transformHeaps[transformHeapIndex];
	int transformIndex = transformHeap.alloc();
	DebugAssert(transformIndex >= 0);

	const EntityDefID defID = initInfo.defID;
	entityInst.init(instID, defID, positionID, bboxID, transformHeapIndex, transformIndex);

	WorldDouble3 &entityPosition = this->positions.get(positionID);
	entityPosition = initInfo.feetPosition;

	// Worst case 3D dimensions.
	double animMaxWidth, animMaxHeight;
	EntityUtils::getAnimationMaxDims(animDef, &animMaxWidth, &animMaxHeight);
	const double halfAnimMaxWidth = animMaxWidth * 0.50;

	// Center bbox in model space.
	const WorldDouble3 entityBBoxMin(-halfAnimMaxWidth, 0.0, -halfAnimMaxWidth);
	const WorldDouble3 entityBBoxMax(halfAnimMaxWidth, animMaxHeight, halfAnimMaxWidth);
	BoundingBox3D &entityBBox = this->boundingBoxes.get(bboxID);
	entityBBox.init(entityBBoxMin, entityBBoxMax);

	entityInst.animInstID = this->animInsts.alloc();
	if (entityInst.animInstID < 0)
	{
		DebugLogError("Couldn't allocate EntityAnimationInstanceID.");
	}

	EntityAnimationInstance &animInst = this->animInsts.get(entityInst.animInstID);
	for (int animDefStateIndex = 0; animDefStateIndex < animDef.stateCount; animDefStateIndex++)
	{
		const EntityAnimationDefinitionState &animDefState = animDef.states[animDefStateIndex];
		animInst.addState(animDefState.seconds, animDefState.isLooping);
	}

	animInst.setStateIndex(initInfo.initialAnimStateIndex);

	if (!TryCreatePhysicsCollider(entityPosition, animMaxHeight, initInfo.isSensorCollider, physicsSystem, &entityInst.physicsBodyID))
	{
		DebugLogError("Couldn't allocate entity Jolt physics body.");
	}

	if (initInfo.canBeKilled)
	{
		entityInst.combatStateID = this->combatStates.alloc();
		if (entityInst.combatStateID < 0)
		{
			DebugLogError("Couldn't allocate EntityCombatStateID.");
		}

		EntityCombatState &combatState = this->combatStates.get(entityInst.combatStateID);
		combatState.isDying = false;
		combatState.isDead = false;
	}

	if (initInfo.direction.has_value())
	{
		entityInst.directionID = this->directions.alloc();
		if (entityInst.directionID < 0)
		{
			DebugLogError("Couldn't allocate EntityDirectionID.");
		}

		const Double2 &direction = *initInfo.direction;
		this->directions.get(entityInst.directionID) = direction;
	}

	if (initInfo.citizenDirectionIndex.has_value())
	{
		entityInst.citizenDirectionIndexID = this->citizenDirectionIndices.alloc();
		if (entityInst.citizenDirectionIndexID < 0)
		{
			DebugLogError("Couldn't allocate EntityCitizenDirectionIndexID.");
		}

		const uint8_t citizenDirectionIndex = *initInfo.citizenDirectionIndex;
		this->citizenDirectionIndices.get(entityInst.citizenDirectionIndexID) = citizenDirectionIndex;
	}

	if (initInfo.citizenName.has_value())
	{
		entityInst.citizenNameID = this->citizenNames.alloc();
		if (entityInst.citizenNameID < 0)
		{
			DebugLogError("Couldn't allocate EntityCitizenNameID.");
		}

		const EntityCitizenName &citizenName = *initInfo.citizenName;
		this->citizenNames.get(entityInst.citizenNameID) = citizenName;
	}

	if (initInfo.citizenColorSeed.has_value())
	{
		entityInst.paletteIndicesInstID = this->paletteIndices.alloc();
		if (entityInst.paletteIndicesInstID < 0)
		{
			DebugLogError("Couldn't allocate EntityPaletteIndicesInstanceID.");
		}

		const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();

		DebugAssert(initInfo.raceID.has_value());
		const uint16_t citizenColorSeed = *initInfo.citizenColorSeed;
		PaletteIndices &paletteIndices = this->paletteIndices.get(entityInst.paletteIndicesInstID);
		paletteIndices = ArenaAnimUtils::transformCitizenColors(*initInfo.raceID, citizenColorSeed, binaryAssetLibrary.getExeData());
	}

	if (initInfo.hasInventory)
	{
		entityInst.itemInventoryInstID = this->itemInventories.alloc();
		if (entityInst.itemInventoryInstID < 0)
		{
			DebugCrash("Couldn't allocate EntityItemInventoryInstanceID.");
		}

		if (entityDef.type == EntityDefinitionType::Enemy)
		{
			const EnemyEntityDefinition &enemyDef = entityDef.enemy;
			if (enemyDef.type == EnemyEntityDefinitionType::Creature)
			{
				// Creatures have chances to have items added to their inventory according to their lootChances value.
				const ItemLibrary &itemLibrary = ItemLibrary::getInstance();
				std::vector<ItemDefinitionID> testItemDefIDs;

				ItemInventory &itemInventory = this->itemInventories.get(entityInst.itemInventoryInstID);
				int randomItemIndex;
				ItemDefinitionID testItemDefID;

				if (ArenaEntityUtils::getCreatureHasMagicItem(enemyDef.creature.level, enemyDef.creature.lootChances, random))
				{
					testItemDefIDs = itemLibrary.getDefinitionIndicesIf(
						[](const ItemDefinition &itemDef)
					{
						return ItemTypeFlags(itemDef.type).any(ItemType::Accessory | ItemType::Consumable | ItemType::Trinket);
					});

					randomItemIndex = random.next(static_cast<int>(testItemDefIDs.size()));
					testItemDefID = testItemDefIDs[randomItemIndex];
					itemInventory.insert(testItemDefID);
				}

				if (ArenaEntityUtils::getCreatureHasNonMagicWeaponOrArmor(enemyDef.creature.lootChances, random))
				{
					testItemDefIDs = itemLibrary.getDefinitionIndicesIf(
						[](const ItemDefinition &itemDef)
					{
						return ItemTypeFlags(itemDef.type).any(ItemType::Weapon | ItemType::Armor | ItemType::Shield);
					});

					randomItemIndex = random.next(static_cast<int>(testItemDefIDs.size()));
					testItemDefID = testItemDefIDs[randomItemIndex];
					itemInventory.insert(testItemDefID);
				}

				if (ArenaEntityUtils::getCreatureHasMagicWeaponOrArmor(enemyDef.creature.level, enemyDef.creature.lootChances, random))
				{
					testItemDefIDs = itemLibrary.getDefinitionIndicesIf(
						[](const ItemDefinition &itemDef)
					{
						// @todo get one that's actually magic
						return ItemTypeFlags(itemDef.type).any(ItemType::Weapon | ItemType::Armor | ItemType::Shield);
					});

					randomItemIndex = random.next(static_cast<int>(testItemDefIDs.size()));
					testItemDefID = testItemDefIDs[randomItemIndex];
					itemInventory.insert(testItemDefID);
				}
			}
		}
		else
		{
			const int testItemCount = random.next(4); // Can be empty.
			if (testItemCount > 0)
			{
				// @todo: figure out passing in ItemDefinitionIDs with initInfo once doing item tables etc
				const ItemLibrary &itemLibrary = ItemLibrary::getInstance();
				const std::vector<ItemDefinitionID> testItemDefIDs = itemLibrary.getDefinitionIndicesIf(
					[](const ItemDefinition &itemDef)
				{
					return itemDef.type != ItemType::Misc; // Don't want quest items.
				});

				ItemInventory &itemInventory = this->itemInventories.get(entityInst.itemInventoryInstID);
				for (int i = 0; i < testItemCount; i++)
				{
					const int randomItemIndex = random.next(static_cast<int>(testItemDefIDs.size()));
					const ItemDefinitionID testItemDefID = testItemDefIDs[randomItemIndex];
					itemInventory.insert(testItemDefID);
				}
			}
		}
	}

	if (initInfo.hasCreatureSound)
	{
		entityInst.creatureSoundInstID = this->creatureSoundInsts.alloc();
		if (entityInst.creatureSoundInstID < 0)
		{
			DebugCrash("Couldn't allocate EntityCreatureSoundInstanceID.");
		}

		double &secondsTillNextCreatureSound = this->creatureSoundInsts.get(entityInst.creatureSoundInstID);
		secondsTillNextCreatureSound = EntityUtils::nextCreatureSoundWaitSeconds(random);
	}

	if (initInfo.isLocked.has_value())
	{
		entityInst.lockStateID = this->lockStates.alloc();
		if (entityInst.lockStateID < 0)
		{
			DebugCrash("Couldn't allocate EntityLockStateID.");
		}

		EntityLockState &lockState = this->lockStates.get(entityInst.lockStateID);
		lockState.isLocked = *initInfo.isLocked;

		const std::optional<int> lockedAnimDefStateIndex = animDef.findStateIndex(EntityAnimationUtils::STATE_LOCKED.c_str());
		const std::optional<int> unlockedAnimDefStateIndex = animDef.findStateIndex(EntityAnimationUtils::STATE_UNLOCKED.c_str());
		DebugAssert(lockedAnimDefStateIndex.has_value());
		DebugAssert(unlockedAnimDefStateIndex.has_value());
		const int activeAnimDefStateIndex = *initInfo.isLocked ? *lockedAnimDefStateIndex : *unlockedAnimDefStateIndex;
		animInst.setStateIndex(activeAnimDefStateIndex);
	}
}

void EntityChunkManager::populateChunkEntities(EntityChunk &entityChunk, const VoxelChunk &voxelChunk,
	const LevelDefinition &levelDefinition, const LevelInfoDefinition &levelInfoDefinition, const WorldInt2 &levelOffset,
	const EntityGenInfo &entityGenInfo, const std::optional<CitizenGenInfo> &citizenGenInfo,
	Random &random, const EntityDefinitionLibrary &entityDefLibrary, JPH::PhysicsSystem &physicsSystem,
	TextureManager &textureManager, Renderer &renderer)
{
	const ChunkInt2 chunkPos = voxelChunk.position;
	const double ceilingScale = levelInfoDefinition.getCeilingScale();
	ArenaRandom arenaRandom(random.next()); // Don't need the one from Game, this is only a cosmetic random	

	SNInt startX, endX;
	int startY, endY;
	WEInt startZ, endZ;
	ChunkUtils::GetWritingRanges(levelOffset, levelDefinition.getWidth(), levelDefinition.getHeight(),
		levelDefinition.getDepth(), &startX, &startY, &startZ, &endX, &endY, &endZ);

	for (int i = 0; i < levelDefinition.getEntityPlacementDefCount(); i++)
	{
		const LevelEntityPlacementDefinition &placementDef = levelDefinition.getEntityPlacementDef(i);
		const LevelVoxelEntityDefID levelEntityDefID = placementDef.id;
		const EntityDefinition &entityDef = levelInfoDefinition.getEntityDef(levelEntityDefID);
		const EntityDefinitionType entityDefType = entityDef.type;

		const int entityDefArenaYOffset = EntityUtils::getYOffset(entityDef);
		const double entityDefYOffset = static_cast<double>(-entityDefArenaYOffset) / MIFUtils::ARENA_UNITS;

		const bool isDynamicEntity = EntityUtils::isDynamicEntity(entityDefType);

		const EntityAnimationDefinition &animDef = entityDef.animDef;
		const char *initialAnimStateName = animDef.initialStateName;
		if (EntityUtils::isStreetlight(entityDef) && entityGenInfo.nightLightsAreActive)
		{
			initialAnimStateName = EntityAnimationUtils::STATE_ACTIVATED.c_str();
		}

		const std::optional<int> initialAnimStateIndex = animDef.findStateIndex(initialAnimStateName);
		DebugAssert(initialAnimStateIndex.has_value());

		std::optional<EntityDefID> entityDefID; // Global entity def ID (shared across all active chunks).
		for (const WorldDouble2 &worldPosition : placementDef.positions)
		{
			const WorldInt2 worldVoxelXZ = VoxelUtils::pointToVoxel(worldPosition);
			const WorldInt3 worldVoxel(worldVoxelXZ.x, 1, worldVoxelXZ.y);
			if (!ChunkUtils::IsInWritingRange(worldVoxel, startX, endX, startY, endY, startZ, endZ))
			{
				continue;
			}

			if (!entityDefID.has_value())
			{
				entityDefID = this->getOrAddEntityDefID(entityDef, entityDefLibrary);
			}

			const CoordDouble2 coordXZ(chunkPos, ChunkUtils::MakeChunkPointFromLevel(worldPosition, startX, startZ));
			const WorldDouble2 worldPositionXZ = VoxelUtils::coordToWorldPoint(coordXZ);
			const VoxelInt3 voxel = VoxelUtils::worldVoxelToCoord(worldVoxel).voxel;
			const VoxelShapeDefID voxelShapeDefID = voxelChunk.shapeDefIDs.get(voxel.x, voxel.y, voxel.z);
			const VoxelShapeDefinition &voxelShapeDef = voxelChunk.shapeDefs[voxelShapeDefID];
			const double feetY = ceilingScale + entityDefYOffset + GetElevatedPlatformHeight(voxelShapeDef, ceilingScale);

			EntityInitInfo initInfo;
			initInfo.defID = *entityDefID;
			initInfo.feetPosition = WorldDouble3(worldPositionXZ.x, feetY, worldPositionXZ.y);
			initInfo.initialAnimStateIndex = *initialAnimStateIndex;
			initInfo.isSensorCollider = !EntityUtils::hasCollision(entityDef);

			if (isDynamicEntity)
			{
				initInfo.direction = CardinalDirection::North;
				initInfo.canBeKilled = entityDefType == EntityDefinitionType::Enemy;
				initInfo.hasCreatureSound = (entityDefType == EntityDefinitionType::Enemy) && (entityDef.enemy.type == EnemyEntityDefinitionType::Creature);
			}

			initInfo.hasInventory = (entityDefType == EntityDefinitionType::Enemy) || (entityDefType == EntityDefinitionType::Container);

			if (entityDefType == EntityDefinitionType::Container)
			{
				const ContainerEntityDefinition &containerDef = entityDef.container;
				if (containerDef.type == ContainerEntityDefinitionType::Holder)
				{
					initInfo.isLocked = containerDef.holder.locked;
				}
			}

			const EntityInstanceID entityInstID = this->entities.alloc();
			if (entityInstID < 0)
			{
				DebugLogError("Couldn't allocate level EntityInstanceID.");
				continue;
			}

			EntityInstance &entityInst = this->entities.get(entityInstID);
			this->initializeEntity(entityInst, entityInstID, entityDef, animDef, initInfo, random, physicsSystem, renderer);
			entityChunk.entityIDs.emplace_back(entityInstID);
		}
	}

	if (citizenGenInfo.has_value())
	{
		auto tryMakeCitizenSpawnVoxel = [&voxelChunk, &random]() -> std::optional<VoxelInt2>
		{
			constexpr int maxSpawnAttemptsCount = 30;
			for (int spawnAttempt = 0; spawnAttempt < maxSpawnAttemptsCount; spawnAttempt++)
			{
				const VoxelInt2 spawnVoxel(random.next(Chunk::WIDTH), random.next(Chunk::DEPTH));
				const VoxelTraitsDefID voxelTraitsDefID = voxelChunk.traitsDefIDs.get(spawnVoxel.x, 1, spawnVoxel.y);
				const VoxelTraitsDefID groundVoxelTraitsDefID = voxelChunk.traitsDefIDs.get(spawnVoxel.x, 0, spawnVoxel.y);
				const VoxelTraitsDefinition &voxelTraitsDef = voxelChunk.traitsDefs[voxelTraitsDefID];
				const VoxelTraitsDefinition &groundVoxelTraitsDef = voxelChunk.traitsDefs[groundVoxelTraitsDefID];
				const bool isValidSpawnVoxel = (voxelTraitsDef.type == ArenaVoxelType::None) && (groundVoxelTraitsDef.type == ArenaVoxelType::Floor);
				if (isValidSpawnVoxel)
				{
					return spawnVoxel;
				}
			}

			return std::nullopt;
		};

		const TextAssetLibrary &textAssetLibrary = TextAssetLibrary::getInstance();

		const int currentCitizenCount = CitizenUtils::getCitizenCount(*this);
		const int targetCitizensToSpawn = std::min(CitizenUtils::MAX_ACTIVE_CITIZENS - currentCitizenCount, CitizenUtils::CITIZENS_PER_CHUNK);
		const int remainingMaleCitizensToSpawn = targetCitizensToSpawn / 2;
		const int remainingFemaleCitizensToSpawn = targetCitizensToSpawn - remainingMaleCitizensToSpawn;
		const int citizenRaceID = citizenGenInfo->raceID;

		const int citizenCountsToSpawn[] = { remainingMaleCitizensToSpawn, remainingFemaleCitizensToSpawn };
		const EntityDefID citizenDefIDs[] = { citizenGenInfo->maleEntityDefID, citizenGenInfo->femaleEntityDefID };
		const EntityDefinition *citizenDefs[] = { citizenGenInfo->maleEntityDef, citizenGenInfo->femaleEntityDef };
		for (int citizenGenderIndex = 0; citizenGenderIndex < 2; citizenGenderIndex++)
		{
			const bool isMale = citizenGenderIndex == 0;
			DebugAssertIndex(citizenCountsToSpawn, citizenGenderIndex);
			const int citizensToSpawn = citizenCountsToSpawn[citizenGenderIndex];
			const EntityDefID citizenEntityDefID = citizenDefIDs[citizenGenderIndex];
			const EntityDefinition &citizenDef = *citizenDefs[citizenGenderIndex];
			const EntityAnimationDefinition &citizenAnimDef = citizenDef.animDef;

			const std::optional<int> initialCitizenAnimStateIndex = citizenAnimDef.findStateIndex(citizenAnimDef.initialStateName);
			DebugAssert(initialCitizenAnimStateIndex.has_value());

			for (int i = 0; i < citizensToSpawn; i++)
			{
				const std::optional<VoxelInt2> citizenSpawnVoxel = tryMakeCitizenSpawnVoxel();
				if (!citizenSpawnVoxel.has_value())
				{
					continue;
				}

				const CoordDouble2 coordXZ(chunkPos, VoxelUtils::getVoxelCenter(*citizenSpawnVoxel));
				const WorldDouble2 worldPositionXZ = VoxelUtils::coordToWorldPoint(coordXZ);

				EntityInitInfo citizenInitInfo;
				citizenInitInfo.defID = citizenEntityDefID;
				citizenInitInfo.feetPosition = WorldDouble3(worldPositionXZ.x, ceilingScale, worldPositionXZ.y);
				citizenInitInfo.initialAnimStateIndex = *initialCitizenAnimStateIndex;
				citizenInitInfo.isSensorCollider = true;
				citizenInitInfo.citizenDirectionIndex = CitizenUtils::getRandomCitizenDirectionIndex(random);

				const std::string citizenNameStr = textAssetLibrary.generateNpcName(citizenRaceID, isMale, arenaRandom);
				citizenInitInfo.citizenName = EntityCitizenName(citizenNameStr.c_str());

				citizenInitInfo.direction = CitizenUtils::getCitizenDirectionByIndex(*citizenInitInfo.citizenDirectionIndex);
				citizenInitInfo.citizenColorSeed = static_cast<uint16_t>(random.next() % std::numeric_limits<uint16_t>::max());
				citizenInitInfo.raceID = citizenRaceID;
				citizenInitInfo.canBeKilled = true;
				citizenInitInfo.hasInventory = false;
				citizenInitInfo.hasCreatureSound = false;

				const EntityInstanceID entityInstID = this->entities.alloc();
				if (entityInstID < 0)
				{
					DebugLogError("Couldn't allocate citizen EntityInstanceID.");
					continue;
				}

				EntityInstance &entityInst = this->entities.get(entityInstID);
				this->initializeEntity(entityInst, entityInstID, citizenDef, citizenAnimDef, citizenInitInfo, random, physicsSystem, renderer);
				entityChunk.entityIDs.emplace_back(entityInstID);
			}
		}
	}
}

void EntityChunkManager::populateChunk(EntityChunk &entityChunk, const VoxelChunk &voxelChunk,
	const LevelDefinition &levelDef, const LevelInfoDefinition &levelInfoDef, const MapSubDefinition &mapSubDef,
	const EntityGenInfo &entityGenInfo, const std::optional<CitizenGenInfo> &citizenGenInfo,
	double ceilingScale, Random &random, const EntityDefinitionLibrary &entityDefLibrary, JPH::PhysicsSystem &physicsSystem,
	TextureManager &textureManager, Renderer &renderer)
{
	const ChunkInt2 chunkPos = entityChunk.position;
	const SNInt levelWidth = levelDef.getWidth();
	const WEInt levelDepth = levelDef.getDepth();

	// Populate all or part of the chunk from a level definition depending on the map type.
	const MapType mapType = mapSubDef.type;
	if (mapType == MapType::Interior)
	{
		DebugAssert(!citizenGenInfo.has_value());

		if (ChunkUtils::touchesLevelDimensions(chunkPos, levelWidth, levelDepth))
		{
			// Populate chunk from the part of the level it overlaps.
			const WorldInt2 levelOffset = chunkPos * ChunkUtils::CHUNK_DIM;
			this->populateChunkEntities(entityChunk, voxelChunk, levelDef, levelInfoDef, levelOffset, entityGenInfo,
				citizenGenInfo, random, entityDefLibrary, physicsSystem, textureManager, renderer);
		}
	}
	else if (mapType == MapType::City)
	{
		DebugAssert(citizenGenInfo.has_value());

		if (ChunkUtils::touchesLevelDimensions(chunkPos, levelWidth, levelDepth))
		{
			// Populate chunk from the part of the level it overlaps.
			const WorldInt2 levelOffset = chunkPos * ChunkUtils::CHUNK_DIM;
			this->populateChunkEntities(entityChunk, voxelChunk, levelDef, levelInfoDef, levelOffset, entityGenInfo,
				citizenGenInfo, random, entityDefLibrary, physicsSystem, textureManager, renderer);
		}
	}
	else if (mapType == MapType::Wilderness)
	{
		DebugAssert(levelDef.getWidth() == Chunk::WIDTH);
		DebugAssert(levelDef.getDepth() == Chunk::DEPTH);
		DebugAssert(citizenGenInfo.has_value());

		// Copy level definition directly into chunk.
		const WorldInt2 levelOffset = WorldInt2::Zero;
		this->populateChunkEntities(entityChunk, voxelChunk, levelDef, levelInfoDef, levelOffset, entityGenInfo,
			citizenGenInfo, random, entityDefLibrary, physicsSystem, textureManager, renderer);
	}
}

void EntityChunkManager::updateCitizenStates(double dt, const WorldDouble2 &playerPositionXZ, bool isPlayerMoving, bool isPlayerWeaponSheathed,
	Random &random, JPH::PhysicsSystem &physicsSystem, const VoxelChunkManager &voxelChunkManager)
{
	JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();

	// @todo now that this entity loop isn't per-chunk, it's possible the citizen starts in a freed chunk this frame and walks to an active chunk
	// despite already being marked for destruction. Ideally would not iterate over all entities (or even all citizens) but a list of citizens
	// not marked for destruction.

	for (const EntityInstance &entityInst : this->entities.values)
	{
		if (!entityInst.isCitizen())
		{
			continue;
		}

		const EntityInstanceID entityInstID = entityInst.instanceID;
		WorldDouble3 &entityPosition = this->positions.get(entityInst.positionID);
		const WorldDouble2 entityPositionXZ = entityPosition.getXZ();
		const ChunkInt2 prevEntityChunkPos = VoxelUtils::worldPointToChunk(entityPositionXZ);
		ChunkInt2 curEntityChunkPos = prevEntityChunkPos; // Potentially updated by entity movement.
		const VoxelDouble2 dirToPlayer = playerPositionXZ - entityPositionXZ;
		const double distToPlayerSqr = dirToPlayer.lengthSquared();

		const EntityDefinition &entityDef = this->getEntityDef(entityInst.defID);
		const EntityAnimationDefinition &animDef = entityDef.animDef;

		const std::optional<int> idleStateIndex = animDef.findStateIndex(EntityAnimationUtils::STATE_IDLE.c_str());
		if (!idleStateIndex.has_value())
		{
			DebugCrash("Couldn't get citizen idle state index.");
		}

		const std::optional<int> walkStateIndex = animDef.findStateIndex(EntityAnimationUtils::STATE_WALK.c_str());
		if (!walkStateIndex.has_value())
		{
			DebugCrash("Couldn't get citizen walk state index.");
		}

		EntityAnimationInstance &animInst = this->animInsts.get(entityInst.animInstID);
		VoxelDouble2 &entityDir = this->directions.get(entityInst.directionID);
		int8_t &citizenDirIndex = this->citizenDirectionIndices.get(entityInst.citizenDirectionIndexID);
		if (animInst.currentStateIndex == idleStateIndex)
		{
			const bool shouldChangeToWalking = !isPlayerWeaponSheathed || (distToPlayerSqr > ArenaCitizenUtils::IDLE_DISTANCE_REAL_SQR) || isPlayerMoving;

			// @todo: need to preserve their previous direction so they stay aligned with
			// the center of the voxel. Basically need to store cardinal direction as internal state.
			if (shouldChangeToWalking)
			{
				animInst.setStateIndex(*walkStateIndex);
				entityDir = CitizenUtils::getCitizenDirectionByIndex(citizenDirIndex);
			}
			else
			{
				// Face towards player.
				// @todo: cache the previous entity dir here so it can be popped when we return to walking. Could maybe have an EntityCitizenDirectionPool that stores ints.
				entityDir = dirToPlayer;
			}
		}
		else if (animInst.currentStateIndex == walkStateIndex)
		{
			const bool shouldChangeToIdle = isPlayerWeaponSheathed && (distToPlayerSqr <= ArenaCitizenUtils::IDLE_DISTANCE_REAL_SQR) && !isPlayerMoving;
			if (shouldChangeToIdle)
			{
				animInst.setStateIndex(*idleStateIndex);
			}
		}

		// Update citizen position and change facing if about to hit something.
		const int curAnimStateIndex = animInst.currentStateIndex;
		if (curAnimStateIndex == *walkStateIndex)
		{
			auto getVoxelAtDistance = [&entityPositionXZ](const VoxelDouble2 &checkDist) -> WorldInt2
			{
				const WorldDouble2 worldPosition = entityPositionXZ + checkDist;
				return VoxelUtils::pointToVoxel(worldPosition);
			};

			const WorldInt2 curWorldVoxel = VoxelUtils::pointToVoxel(entityPositionXZ);
			const WorldInt2 nextWorldVoxel = getVoxelAtDistance(entityDir * 0.50);

			if (nextWorldVoxel != curWorldVoxel)
			{
				auto isSuitableVoxel = [&voxelChunkManager](const WorldInt2 &worldVoxel)
				{
					const CoordInt2 coord = VoxelUtils::worldVoxelToCoord(worldVoxel);
					const VoxelChunk *voxelChunk = voxelChunkManager.findChunkAtPosition(coord.chunk);

					const bool isValidVoxel = voxelChunk != nullptr;
					if (!isValidVoxel)
					{
						return false;
					}

					const VoxelInt3 mainFloorVoxel(coord.voxel.x, 1, coord.voxel.y);
					const VoxelTraitsDefID mainFloorVoxelTraitsDefID = voxelChunk->traitsDefIDs.get(mainFloorVoxel.x, mainFloorVoxel.y, mainFloorVoxel.z);
					const VoxelTraitsDefinition &mainFloorVoxelTraitsDef = voxelChunk->traitsDefs[mainFloorVoxelTraitsDefID];
					const bool isPassableVoxel = mainFloorVoxelTraitsDef.type == ArenaVoxelType::None;
					if (!isPassableVoxel)
					{
						return false;
					}

					const VoxelInt3 floorVoxel(coord.voxel.x, 0, coord.voxel.y);
					const VoxelTraitsDefID floorVoxelTraitsDefID = voxelChunk->traitsDefIDs.get(floorVoxel.x, floorVoxel.y, floorVoxel.z);
					const VoxelTraitsDefinition &floorVoxelTraitsDef = voxelChunk->traitsDefs[floorVoxelTraitsDefID];
					const bool isWalkableVoxel = floorVoxelTraitsDef.type == ArenaVoxelType::Floor;
					if (!isWalkableVoxel)
					{
						return false;
					}

					return true;
				};

				if (!isSuitableVoxel(nextWorldVoxel))
				{
					// Need to change walking direction. Determine another safe route, or if
					// none exist, then stop walking.
					const CardinalDirectionName curDirectionName = CardinalDirection::getDirectionName(entityDir);

					// Shuffle citizen direction indices so they don't all switch to the same direction every time.
					constexpr auto &dirIndices = ArenaCitizenUtils::DIRECTION_INDICES;
					int8_t randomDirectionIndices[std::size(dirIndices)];
					std::copy(std::begin(dirIndices), std::end(dirIndices), std::begin(randomDirectionIndices));
					RandomUtils::shuffle<int8_t>(randomDirectionIndices, random);

					const int8_t *indicesBegin = std::begin(randomDirectionIndices);
					const int8_t *indicesEnd = std::end(randomDirectionIndices);
					const auto iter = std::find_if(indicesBegin, indicesEnd,
						[&getVoxelAtDistance, &isSuitableVoxel, curDirectionName](int8_t dirIndex)
					{
						// See if this is a valid direction to go in.
						const CardinalDirectionName cardinalDirectionName = CitizenUtils::getCitizenDirectionNameByIndex(dirIndex);
						if (cardinalDirectionName != curDirectionName)
						{
							const WorldDouble2 &possibleDirection = CitizenUtils::getCitizenDirectionByIndex(dirIndex);
							const WorldInt2 possibleVoxel = getVoxelAtDistance(possibleDirection * 0.50);
							if (isSuitableVoxel(possibleVoxel))
							{
								return true;
							}
						}

						return false;
					});

					if (iter != indicesEnd)
					{
						citizenDirIndex = *iter;
						entityDir = CitizenUtils::getCitizenDirectionByIndex(citizenDirIndex);
					}
					else
					{
						// Couldn't find any valid direction. The citizen is probably stuck somewhere.
					}
				}
			}

			// Integrate by delta time.
			const VoxelDouble2 entityVelocity = entityDir * ArenaCitizenUtils::MOVE_SPEED_PER_SECOND;
			const WorldDouble2 newEntityPositionXZ = entityPositionXZ + (entityVelocity * dt);
			entityPosition.x = newEntityPositionXZ.x;
			entityPosition.z = newEntityPositionXZ.y;
			curEntityChunkPos = VoxelUtils::worldPointToChunk(newEntityPositionXZ);

			const JPH::BodyID &physicsBodyID = entityInst.physicsBodyID;
			DebugAssert(!physicsBodyID.IsInvalid());

			const JPH::RVec3 oldBodyPosition = bodyInterface.GetPosition(physicsBodyID);
			const JPH::RVec3 newBodyPosition(
				static_cast<float>(newEntityPositionXZ.x),
				static_cast<float>(oldBodyPosition.GetY()),
				static_cast<float>(newEntityPositionXZ.y));
			bodyInterface.SetPosition(physicsBodyID, newBodyPosition, JPH::EActivation::Activate);
		}

		// Transfer ownership of the entity ID to a new chunk if needed.
		if (curEntityChunkPos != prevEntityChunkPos)
		{
			EntityChunk *prevEntityChunk = this->findChunkAtPosition(prevEntityChunkPos); // Citizen may have crossed chunk boundary same frame as player.
			EntityChunk *curEntityChunk = this->findChunkAtPosition(curEntityChunkPos);

			if (prevEntityChunk != nullptr)
			{
				std::vector<EntityInstanceID> &prevEntityChunkIDs = prevEntityChunk->entityIDs;
				for (int entityIndex = 0; entityIndex < static_cast<int>(prevEntityChunkIDs.size()); entityIndex++)
				{
					const EntityInstanceID prevEntityChunkInstID = prevEntityChunkIDs[entityIndex];
					if (prevEntityChunkInstID == entityInstID)
					{
						prevEntityChunkIDs.erase(prevEntityChunkIDs.begin() + entityIndex);
						break;
					}
				}
			}

			if (curEntityChunk != nullptr)
			{
				const auto destroyedIter = std::find(this->destroyedEntityIDs.begin(), this->destroyedEntityIDs.end(), entityInstID);
				const bool isCitizenDestroyedThisFrame = destroyedIter != this->destroyedEntityIDs.end();
				if (!isCitizenDestroyedThisFrame)
				{
					curEntityChunk->entityIDs.emplace_back(entityInstID);

					EntityTransferResult transferResult;
					transferResult.id = entityInstID;
					transferResult.oldChunkPos = prevEntityChunkPos;
					transferResult.newChunkPos = curEntityChunkPos;
					this->transferResults.emplace_back(std::move(transferResult));
				}
			}
		}
	}
}

std::string EntityChunkManager::getCreatureSoundFilename(const EntityDefID defID) const
{
	const EntityDefinition &entityDef = this->getEntityDef(defID);
	if (entityDef.type != EntityDefinitionType::Enemy)
	{
		return std::string();
	}

	const EnemyEntityDefinition &enemyDef = entityDef.enemy;
	if (enemyDef.type != EnemyEntityDefinitionType::Creature)
	{
		return std::string();
	}

	const EnemyEntityDefinition::CreatureDefinition &creatureDef = enemyDef.creature;
	const std::string_view creatureSoundName = creatureDef.soundName;
	return String::toUppercase(std::string(creatureSoundName));
}

const EntityInstance &EntityChunkManager::getEntity(EntityInstanceID id) const
{
	return this->entities.get(id);
}

const WorldDouble3 &EntityChunkManager::getEntityPosition(EntityPositionID id) const
{
	return this->positions.get(id);
}

const BoundingBox3D &EntityChunkManager::getEntityBoundingBox(EntityBoundingBoxID id) const
{
	return this->boundingBoxes.get(id);
}

const Double2 &EntityChunkManager::getEntityDirection(EntityDirectionID id) const
{
	return this->directions.get(id);
}

EntityAnimationInstance &EntityChunkManager::getEntityAnimationInstance(EntityAnimationInstanceID id)
{
	return this->animInsts.get(id);
}

const EntityAnimationInstance &EntityChunkManager::getEntityAnimationInstance(EntityAnimationInstanceID id) const
{
	return this->animInsts.get(id);
}

EntityCombatState &EntityChunkManager::getEntityCombatState(EntityCombatStateID id)
{
	return this->combatStates.get(id);
}

const EntityCombatState &EntityChunkManager::getEntityCombatState(EntityCombatStateID id) const
{
	return this->combatStates.get(id);
}

int8_t EntityChunkManager::getEntityCitizenDirectionIndex(EntityCitizenDirectionIndexID id) const
{
	return this->citizenDirectionIndices.get(id);
}

const EntityCitizenName &EntityChunkManager::getEntityCitizenName(EntityCitizenNameID id) const
{
	return this->citizenNames.get(id);
}

const PaletteIndices &EntityChunkManager::getEntityPaletteIndices(EntityPaletteIndicesInstanceID id) const
{
	return this->paletteIndices.get(id);
}

ItemInventory &EntityChunkManager::getEntityItemInventory(EntityItemInventoryInstanceID id)
{
	return this->itemInventories.get(id);
}

EntityLockState &EntityChunkManager::getEntityLockState(EntityLockStateID id)
{
	return this->lockStates.get(id);
}

const EntityLockState &EntityChunkManager::getEntityLockState(EntityLockStateID id) const
{
	return this->lockStates.get(id);
}

EntityInstanceID EntityChunkManager::getEntityFromPhysicsBodyID(JPH::BodyID bodyID) const
{
	if (bodyID.IsInvalid())
	{
		return -1;
	}

	// @todo: probably want a smarter lookup than this
	for (const EntityInstance &entityInst : this->entities.values)
	{
		if (entityInst.physicsBodyID == bodyID)
		{
			return entityInst.instanceID;
		}
	}

	return -1;
}

int EntityChunkManager::getCountInChunkWithDirection(const ChunkInt2 &chunkPos) const
{
	const int chunkIndex = this->findChunkIndex(chunkPos);
	if (chunkIndex < 0)
	{
		DebugLogWarningFormat("Missing chunk (%s) for counting entities with direction.", chunkPos.toString().c_str());
		return 0;
	}

	const EntityChunk &chunk = this->getChunkAtIndex(chunkIndex);

	int count = 0;
	for (const EntityInstanceID entityInstID : chunk.entityIDs)
	{
		const EntityInstance &entityInst = this->entities.get(entityInstID);
		if (entityInst.directionID >= 0)
		{
			count++;
		}
	}

	return count;
}

int EntityChunkManager::getCountInChunkWithCreatureSound(const ChunkInt2 &chunkPos) const
{
	const int chunkIndex = this->findChunkIndex(chunkPos);
	if (chunkIndex < 0)
	{
		DebugLogWarningFormat("Missing chunk (%s) for counting entities with creature sound.", chunkPos.toString().c_str());
		return 0;
	}

	const EntityChunk &chunk = this->getChunkAtIndex(chunkIndex);

	int count = 0;
	for (const EntityInstanceID entityInstID : chunk.entityIDs)
	{
		const EntityInstance &entityInst = this->entities.get(entityInstID);
		if (entityInst.creatureSoundInstID >= 0)
		{
			count++;
		}
	}

	return count;
}

int EntityChunkManager::getCountInChunkWithCitizenDirection(const ChunkInt2 &chunkPos) const
{
	const int chunkIndex = this->findChunkIndex(chunkPos);
	if (chunkIndex < 0)
	{
		DebugLogWarningFormat("Missing chunk (%s) for counting entities with citizen direction.", chunkPos.toString().c_str());
		return 0;
	}

	const EntityChunk &chunk = this->getChunkAtIndex(chunkIndex);

	int count = 0;
	for (const EntityInstanceID entityInstID : chunk.entityIDs)
	{
		const EntityInstance &entityInst = this->entities.get(entityInstID);
		if (entityInst.citizenDirectionIndexID >= 0)
		{
			count++;
		}
	}

	return count;
}

Span<const EntityInstanceID> EntityChunkManager::getQueuedDestroyEntityIDs() const
{
	return this->destroyedEntityIDs;
}

Span<RenderTransformHeap> EntityChunkManager::getTransformHeaps()
{
	return this->transformHeaps;
}

Span<const EntityTransferResult> EntityChunkManager::getEntityTransferResults() const
{
	return this->transferResults;
}

void EntityChunkManager::getEntityObservedResult(EntityInstanceID id, const WorldDouble3 &eyePosition, EntityObservedResult &result) const
{
	const EntityInstance &entityInst = this->entities.get(id);
	const EntityDefinition &entityDef = this->getEntityDef(entityInst.defID);
	const EntityAnimationDefinition &animDef = entityDef.animDef;
	const EntityAnimationInstance &animInst = this->animInsts.get(entityInst.animInstID);

	const WorldDouble2 eyePositionXZ = eyePosition.getXZ();
	const WorldDouble3 entityPosition = this->positions.get(entityInst.positionID);
	const WorldDouble2 entityPositionXZ = entityPosition.getXZ();

	const int stateIndex = animInst.currentStateIndex;
	DebugAssert(stateIndex >= 0);
	DebugAssert(stateIndex < animDef.stateCount);
	const EntityAnimationDefinitionState &animDefState = animDef.states[stateIndex];
	const int angleCount = animDefState.keyframeListCount;

	// Get animation angle based on relative facing to camera. Static entities always face the camera.
	Radians animAngle = 0.0;
	if (entityInst.isDynamic())
	{
		const VoxelDouble2 &entityDir = this->getEntityDirection(entityInst.directionID);
		const VoxelDouble2 diffDir = (eyePositionXZ - entityPositionXZ).normalized();
		const Radians entityAngle = MathUtils::fullAtan2(entityDir);
		const Radians diffAngle = MathUtils::fullAtan2(diffDir);
		const Radians relativeAngle = Constants::TwoPi + (entityAngle - diffAngle);

		// Keep final direction centered within its angle range.
		const Radians angleBias = (Constants::TwoPi / static_cast<double>(angleCount)) * 0.50;

		animAngle = std::fmod(relativeAngle + angleBias, Constants::TwoPi);
	}

	// Get current keyframe list.
	const double angleCountReal = static_cast<double>(angleCount);
	const double anglePercent = animAngle / Constants::TwoPi;
	const int unclampedAngleIndex = static_cast<int>(angleCountReal * anglePercent);
	const int angleIndex = std::clamp(unclampedAngleIndex, 0, angleCount - 1);
	const int animDefKeyframeListIndex = animDefState.keyframeListsIndex + angleIndex;
	DebugAssert(angleIndex >= 0);
	DebugAssert(angleIndex < animDefState.keyframeListCount);
	DebugAssert(animDefKeyframeListIndex < animDef.keyframeListCount);
	const EntityAnimationDefinitionKeyframeList &animDefKeyframeList = animDef.keyframeLists[animDefKeyframeListIndex];

	// Get current keyframe.
	const int frameCount = animDefKeyframeList.keyframeCount;
	const double frameCountReal = static_cast<double>(frameCount);
	const int unclampedFrameIndex = static_cast<int>(frameCountReal * animInst.progressPercent);
	const int frameIndex = std::clamp(unclampedFrameIndex, 0, frameCount - 1);
	const int animDefKeyframeIndex = animDefKeyframeList.keyframesIndex + frameIndex;
	DebugAssert(frameIndex >= 0);
	DebugAssert(frameIndex < frameCount);
	DebugAssert(animDefKeyframeIndex < animDef.keyframeCount);
	const EntityAnimationDefinitionKeyframe &animDefKeyframe = animDef.keyframes[animDefKeyframeIndex];
	const int linearizedKeyframeIndex = animDefKeyframe.linearizedIndex;
	DebugAssert(linearizedKeyframeIndex >= 0);
	DebugAssert(linearizedKeyframeIndex < animDef.keyframeCount);

	result.init(id, linearizedKeyframeIndex);
}

void EntityChunkManager::updateCreatureSounds(double dt, const WorldDouble3 &playerPosition, Random &random, AudioManager &audioManager)
{
	for (EntityInstance &entityInst : this->entities.values)
	{
		if (entityInst.creatureSoundInstID >= 0)
		{
			const EntityCombatState &combatState = this->combatStates.get(entityInst.combatStateID);
			if (combatState.isInDeathState())
			{
				continue;
			}

			double &secondsTillCreatureSound = this->creatureSoundInsts.get(entityInst.creatureSoundInstID);
			secondsTillCreatureSound -= dt;
			if (secondsTillCreatureSound <= 0.0)
			{
				const WorldDouble3 entityPosition = this->positions.get(entityInst.positionID);
				const BoundingBox3D &entityBBox = this->boundingBoxes.get(entityInst.bboxID);
				const WorldDouble3 entitySoundPosition(entityPosition.x, entityPosition.y + entityBBox.halfHeight, entityPosition.z);
				if (EntityUtils::withinHearingDistance(playerPosition, entitySoundPosition))
				{
					// @todo: store some kind of sound def ID w/ the secondsTillCreatureSound instead of generating the sound filename here.
					const std::string creatureSoundFilename = this->getCreatureSoundFilename(entityInst.defID);
					if (creatureSoundFilename.empty())
					{
						continue;
					}

					// Center the sound inside the creature.
					audioManager.playSound(creatureSoundFilename.c_str(), entitySoundPosition);

					secondsTillCreatureSound = EntityUtils::nextCreatureSoundWaitSeconds(random);
				}
			}
		}
	}
}

void EntityChunkManager::updateFadedElevatedPlatforms(EntityChunk &entityChunk, const VoxelChunk &voxelChunk, double ceilingScale, JPH::PhysicsSystem &physicsSystem)
{
	for (const VoxelFadeAnimationInstance &fadeAnimInst : voxelChunk.fadeAnimInsts)
	{
		if (fadeAnimInst.isDoneFading())
		{
			for (int i = static_cast<int>(entityChunk.entityIDs.size()) - 1; i >= 0; i--)
			{
				const EntityInstanceID entityInstID = entityChunk.entityIDs[i];
				const EntityInstance &entityInst = this->entities.get(entityInstID);
				WorldDouble3 &entityPosition = this->positions.get(entityInst.positionID);
				WorldInt3 entityWorldVoxel = VoxelUtils::pointToVoxel(entityPosition, ceilingScale);
				const CoordInt3 entityVoxelCoord = VoxelUtils::worldVoxelToCoord(entityWorldVoxel);
				const VoxelInt3 entityVoxel = entityVoxelCoord.voxel;
				const bool matchesFadedVoxel = (entityVoxel.x == fadeAnimInst.x) && (entityVoxel.y == fadeAnimInst.y) && (entityVoxel.z == fadeAnimInst.z);

				// @todo: we don't know if this was a raised platform because the voxel shape has already changed this frame, so just assume yes for "can be elevated" entities
				if (matchesFadedVoxel && entityInst.canUseElevatedPlatforms())
				{
					JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();
					const JPH::BodyID entityPhysicsBodyID = entityInst.physicsBodyID;
					const JPH::RVec3 oldEntityPhysicsPosition = bodyInterface.GetPosition(entityPhysicsBodyID);
					const JPH::ShapeRefC entityPhysicsShape = bodyInterface.GetShape(entityPhysicsBodyID);
					const JPH::AABox entityColliderBBox = entityPhysicsShape->GetLocalBounds();
					const float entityColliderHeight = entityColliderBBox.GetSize().GetY();
					const double newEntityFeetY = ceilingScale;
					entityPosition.y = newEntityFeetY; // Probably don't need entity def Y offset

					const double newEntityPhysicsCenterY = newEntityFeetY + (entityColliderHeight * 0.50);
					const JPH::RVec3 newEntityPhysicsPosition(oldEntityPhysicsPosition.GetX(), static_cast<float>(newEntityPhysicsCenterY), oldEntityPhysicsPosition.GetZ());
					bodyInterface.SetPosition(entityPhysicsBodyID, newEntityPhysicsPosition, JPH::EActivation::Activate);
				}
			}
		}
	}
}

void EntityChunkManager::updateEnemyDeathStates(JPH::PhysicsSystem &physicsSystem, AudioManager &audioManager)
{
	JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();

	// @todo: just check an EntityChunkManager::dyingEntities list instead, added to when player swing kills them

	for (EntityInstance &entityInst : this->entities.values)
	{
		const EntityInstanceID entityInstID = entityInst.instanceID;
		const EntityDefinition &entityDef = this->getEntityDef(entityInst.defID);
		const EntityDefinitionType entityDefType = entityDef.type;
		if (!entityInst.canBeKilledInCombat())
		{
			continue;
		}

		const std::optional<int> deathAnimStateIndex = EntityUtils::tryGetDeathAnimStateIndex(entityDef.animDef);
		if (!deathAnimStateIndex.has_value())
		{
			continue;
		}

		EntityAnimationInstance &animInst = this->animInsts.get(entityInst.animInstID);
		const bool isInDeathAnimState = animInst.currentStateIndex == *deathAnimStateIndex;
		if (!isInDeathAnimState)
		{
			continue;
		}

		EntityCombatState &combatState = this->combatStates.get(entityInst.combatStateID);
		const bool isDeathAnimComplete = animInst.progressPercent == 1.0;
		if (isDeathAnimComplete)
		{
			if (!combatState.isDead)
			{
				combatState.isDying = false;
				combatState.isDead = true;

				if (EntityUtils::leavesCorpse(entityDef))
				{
					JPH::BodyID &physicsBodyID = entityInst.physicsBodyID;
					if (!physicsBodyID.IsInvalid())
					{
						bodyInterface.RemoveBody(physicsBodyID);
						bodyInterface.DestroyBody(physicsBodyID);
						physicsBodyID = Physics::INVALID_BODY_ID;
					}
				}
				else
				{
					this->queueEntityDestroy(entityInstID, true);
					// @todo remove from dyingEntities list once that is a thing
				}
			}
		}
		else
		{
			if (!combatState.isDying)
			{
				combatState.isDying = true;

				if (EntityUtils::leavesCorpse(entityDef))
				{
					const WorldDouble3 entityPosition = this->positions.get(entityInst.positionID);
					audioManager.playSound(ArenaSoundName::BodyFall, entityPosition);
				}
			}
		}
	}
}

void EntityChunkManager::updateVfx()
{
	for (const EntityInstance &entityInst : this->entities.values)
	{
		const EntityInstanceID entityInstID = entityInst.instanceID;
		const EntityDefinition &entityDef = this->getEntityDef(entityInst.defID);
		const EntityDefinitionType entityDefType = entityDef.type;
		if (entityDefType != EntityDefinitionType::Vfx)
		{
			continue;
		}

		const EntityAnimationInstance &animInst = this->animInsts.get(entityInst.animInstID);
		const bool isVfxAnimComplete = animInst.progressPercent == 1.0;
		if (isVfxAnimComplete)
		{
			this->queueEntityDestroy(entityInstID, true); // @todo shouldn't need to notify chunk, it should just be a loose entity in entitychunkmanager
		}
	}
}

EntityInstanceID EntityChunkManager::createEntity(const EntityInitInfo &initInfo, Random &random, JPH::PhysicsSystem &physicsSystem, Renderer &renderer)
{
	const EntityInstanceID entityInstID = this->entities.alloc();
	if (entityInstID < 0)
	{
		DebugLogError("Couldn't allocate EntityInstanceID.");
		return -1;
	}

	EntityInstance &entityInst = this->entities.get(entityInstID);

	// Register with chunk if possible.
	// @todo: not all entities should need registering, like vfx
	const ChunkInt2 chunkPos = VoxelUtils::worldPointToChunk(initInfo.feetPosition);
	EntityChunk *entityChunk = this->findChunkAtPosition(chunkPos);
	if (entityChunk != nullptr)
	{
		entityChunk->entityIDs.emplace_back(entityInstID);
	}

	const EntityDefinition &entityDef = this->getEntityDef(initInfo.defID);
	this->initializeEntity(entityInst, entityInstID, entityDef, entityDef.animDef, initInfo, random, physicsSystem, renderer);

	return entityInstID;
}

void EntityChunkManager::update(double dt, Span<const ChunkInt2> activeChunkPositions, Span<const ChunkInt2> newChunkPositions,
	Span<const ChunkInt2> freedChunkPositions, const Player &player, const LevelDefinition *activeLevelDef,
	const LevelInfoDefinition *activeLevelInfoDef, const MapSubDefinition &mapSubDef, Span<const LevelDefinition> levelDefs,
	Span<const int> levelInfoDefIndices, Span<const LevelInfoDefinition> levelInfoDefs, const EntityGenInfo &entityGenInfo,
	const std::optional<CitizenGenInfo> &citizenGenInfo, double ceilingScale, Random &random, const VoxelChunkManager &voxelChunkManager,
	AudioManager &audioManager, JPH::PhysicsSystem &physicsSystem, TextureManager &textureManager, Renderer &renderer)
{
	const EntityDefinitionLibrary &entityDefLibrary = EntityDefinitionLibrary::getInstance();

	for (const ChunkInt2 chunkPos : freedChunkPositions)
	{
		const int chunkIndex = this->getChunkIndex(chunkPos);
		const EntityChunk &entityChunk = this->getChunkAtIndex(chunkIndex);
		for (const EntityInstanceID entityInstID : entityChunk.entityIDs)
		{
			this->queueEntityDestroy(entityInstID, false);
		}

		this->recycleChunk(chunkIndex);
	}

	const MapType mapType = mapSubDef.type;
	for (const ChunkInt2 chunkPos : newChunkPositions)
	{
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		const int spawnIndex = this->spawnChunk();
		EntityChunk &entityChunk = this->getChunkAtIndex(spawnIndex);
		entityChunk.init(chunkPos, voxelChunk.height);

		// Default to the active level def unless it's the wilderness which relies on this chunk coordinate.
		const LevelDefinition *levelDefPtr = activeLevelDef;
		const LevelInfoDefinition *levelInfoDefPtr = activeLevelInfoDef;
		if (mapType == MapType::Wilderness)
		{
			const MapDefinitionWild &mapDefWild = mapSubDef.wild;
			const int levelDefIndex = mapDefWild.getLevelDefIndex(chunkPos);
			levelDefPtr = &levelDefs[levelDefIndex];

			const int levelInfoDefIndex = levelInfoDefIndices[levelDefIndex];
			levelInfoDefPtr = &levelInfoDefs[levelInfoDefIndex];
		}

		this->populateChunk(entityChunk, voxelChunk, *levelDefPtr, *levelInfoDefPtr, mapSubDef, entityGenInfo, citizenGenInfo,
			ceilingScale, random, entityDefLibrary, physicsSystem, textureManager, renderer);
	}

	// Free any unneeded chunks for memory savings in case the chunk distance was once large
	// and is now small. This is significant even for chunk distance 2->1, or 25->9 chunks.
	this->chunkPool.clear();

	const WorldDouble3 playerPosition = player.getEyePosition();
	const WorldDouble2 playerPositionXZ = playerPosition.getXZ();
	const bool isPlayerMoving = player.isMoving();

	const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
	const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(player.weaponAnimDefID);
	const WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
	const WeaponAnimationDefinitionState &weaponAnimDefState = weaponAnimDef.states[weaponAnimInst.currentStateIndex];
	const bool isPlayerWeaponSheathed = WeaponAnimationUtils::isSheathed(weaponAnimDefState);

	// @todo: this could support entities not registered to a chunk if we iterate over categories of entityInstIDs instead (all citizens, then all creatures, etc)
	// - at some point may want to store an EntityInstance bool like "isArbitrarySpawn" or something that says "I don't despawn with a chunk" for vfx and temporaries

	for (const ChunkInt2 chunkPos : activeChunkPositions)
	{
		const int chunkIndex = this->getChunkIndex(chunkPos);
		EntityChunk &entityChunk = this->getChunkAtIndex(chunkIndex);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		this->updateFadedElevatedPlatforms(entityChunk, voxelChunk, ceilingScale, physicsSystem);
	}

	for (const EntityInstance &entityInst : this->entities.values)
	{
		EntityAnimationInstance &animInst = this->animInsts.get(entityInst.animInstID);
		animInst.update(dt);
	}

	this->updateCitizenStates(dt, playerPositionXZ, isPlayerMoving, isPlayerWeaponSheathed, random, physicsSystem, voxelChunkManager);
	this->updateCreatureSounds(dt, playerPosition, random, audioManager);
	this->updateEnemyDeathStates(physicsSystem, audioManager);
	this->updateVfx();
}

void EntityChunkManager::queueEntityDestroy(EntityInstanceID entityInstID, const ChunkInt2 *chunkToNotify)
{
	const auto iter = std::find(this->destroyedEntityIDs.begin(), this->destroyedEntityIDs.end(), entityInstID);
	if (iter == this->destroyedEntityIDs.end())
	{
		this->destroyedEntityIDs.emplace_back(entityInstID);

		if (chunkToNotify != nullptr)
		{
			EntityChunk &entityChunk = this->getChunkAtPosition(*chunkToNotify);
			const auto iter = std::find(entityChunk.entityIDs.begin(), entityChunk.entityIDs.end(), entityInstID);
			DebugAssert(iter != entityChunk.entityIDs.end());
			entityChunk.entityIDs.erase(iter);
		}
	}
}

void EntityChunkManager::queueEntityDestroy(EntityInstanceID entityInstID, bool notifyChunk)
{
	const ChunkInt2 *chunkToNotify = nullptr;

	ChunkInt2 chunkPos;
	if (notifyChunk)
	{
		const EntityInstance &entityInst = this->entities.get(entityInstID);
		const WorldDouble3 entityPosition = this->positions.get(entityInst.positionID);
		chunkPos = VoxelUtils::worldPointToChunk(entityPosition);
		chunkToNotify = &chunkPos;
	}

	this->queueEntityDestroy(entityInstID, chunkToNotify);
}

void EntityChunkManager::endFrame(JPH::PhysicsSystem &physicsSystem, Renderer &renderer)
{
	JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();

	for (const EntityInstanceID entityInstID : this->destroyedEntityIDs)
	{
		const EntityInstance &entityInst = this->entities.get(entityInstID);

		if (entityInst.positionID >= 0)
		{
			this->positions.free(entityInst.positionID);
		}

		if (entityInst.bboxID >= 0)
		{
			this->boundingBoxes.free(entityInst.bboxID);
		}

		if (entityInst.directionID >= 0)
		{
			this->directions.free(entityInst.directionID);
		}

		if (entityInst.animInstID >= 0)
		{
			this->animInsts.free(entityInst.animInstID);
		}

		if (entityInst.combatStateID >= 0)
		{
			this->combatStates.free(entityInst.combatStateID);
		}

		if (entityInst.creatureSoundInstID >= 0)
		{
			this->creatureSoundInsts.free(entityInst.creatureSoundInstID);
		}

		if (entityInst.citizenDirectionIndexID >= 0)
		{
			this->citizenDirectionIndices.free(entityInst.citizenDirectionIndexID);
		}

		if (entityInst.citizenNameID >= 0)
		{
			this->citizenNames.free(entityInst.citizenNameID);
		}

		if (entityInst.paletteIndicesInstID >= 0)
		{
			this->paletteIndices.free(entityInst.paletteIndicesInstID);
		}

		if (entityInst.itemInventoryInstID >= 0)
		{
			this->itemInventories.free(entityInst.itemInventoryInstID);
		}

		if (entityInst.lockStateID >= 0)
		{
			this->lockStates.free(entityInst.lockStateID);
		}

		const JPH::BodyID physicsBodyID = entityInst.physicsBodyID;
		if (!physicsBodyID.IsInvalid())
		{
			bodyInterface.RemoveBody(physicsBodyID);
			bodyInterface.DestroyBody(physicsBodyID);
		}

		if (entityInst.transformIndex >= 0)
		{
			RenderTransformHeap &transformHeap = this->transformHeaps[entityInst.transformHeapIndex];
			transformHeap.free(entityInst.transformIndex);
		}

		this->entities.free(entityInstID);
	}

	this->destroyedEntityIDs.clear();
	this->transferResults.clear();
}

void EntityChunkManager::clear(JPH::PhysicsSystem &physicsSystem, Renderer &renderer)
{
	for (ChunkPtr &chunkPtr : this->activeChunks)
	{
		for (const EntityInstanceID entityInstID : chunkPtr->entityIDs)
		{
			this->queueEntityDestroy(entityInstID, false);
		}
	}

	this->endFrame(physicsSystem, renderer);

	for (RenderTransformHeap &transformHeap : this->transformHeaps)
	{
		renderer.freeUniformBuffer(transformHeap.uniformBufferID);
	}

	this->transformHeaps.clear();

	this->recycleAllChunks();
}
