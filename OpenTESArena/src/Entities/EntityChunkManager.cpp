#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"

#include "ArenaAnimUtils.h"
#include "ArenaCitizenUtils.h"
#include "EntityChunkManager.h"
#include "EntityDefinitionLibrary.h"
#include "EntityObservedResult.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/MIFUtils.h"
#include "../Assets/TextureManager.h"
#include "../Audio/AudioManager.h"
#include "../Collision/PhysicsLayer.h"
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

#include "components/utilities/String.h"

namespace
{
	struct EntityInitInfo
	{
		EntityDefID defID;
		WorldDouble3 feetPosition;
		WorldDouble3 bboxMin, bboxMax; // Centered on the entity in model space
		double animMaxHeight;
		char initialAnimStateIndex;
		bool isSensorCollider;
		std::optional<Double2> direction;
		std::optional<int8_t> citizenDirectionIndex;
		std::optional<uint16_t> citizenColorSeed;
		std::optional<int> raceID;
		bool hasInventory;
		bool hasCreatureSound;

		EntityInitInfo()
		{
			this->defID = -1;
			this->animMaxHeight = 0.0;
			this->initialAnimStateIndex = -1;
			this->isSensorCollider = false;
			this->hasInventory = false;
			this->hasCreatureSound = false;
		}
	};

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
				DebugLogWarning("Couldn't load entity anim texture \"" + textureAsset.filename + "\".");
				continue;
			}

			const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
			ObjectTextureID textureID;
			if (!renderer.tryCreateObjectTexture(textureBuilder, &textureID))
			{
				DebugLogWarning("Couldn't create entity anim texture \"" + textureAsset.filename + "\".");
				continue;
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

EntityInstanceID EntityChunkManager::spawnEntity()
{
	EntityInstanceID instID;
	if (!this->entities.tryAlloc(&instID))
	{
		DebugCrash("Couldn't allocate EntityInstanceID.");
	}

	return instID;
}

void EntityChunkManager::populateChunkEntities(EntityChunk &entityChunk, const VoxelChunk &voxelChunk,
	const LevelDefinition &levelDefinition, const LevelInfoDefinition &levelInfoDefinition, const WorldInt2 &levelOffset,
	const EntityGeneration::EntityGenInfo &entityGenInfo, const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
	Random &random, const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	JPH::PhysicsSystem &physicsSystem, TextureManager &textureManager, Renderer &renderer)
{
	const ChunkInt2 chunkPos = voxelChunk.getPosition();
	const double ceilingScale = levelInfoDefinition.getCeilingScale();

	auto initializeEntity = [this, &random, &binaryAssetLibrary, &physicsSystem, ceilingScale](
		EntityInstance &entityInst, EntityInstanceID instID, const EntityDefinition &entityDef, const EntityInitInfo &initInfo)
	{
		EntityPositionID positionID;
		if (!this->positions.tryAlloc(&positionID))
		{
			DebugLogError("Couldn't allocate EntityPositionID.");
		}

		EntityBoundingBoxID bboxID;
		if (!this->boundingBoxes.tryAlloc(&bboxID))
		{
			DebugLogError("Couldn't allocate EntityBoundingBoxID.");
		}

		const EntityDefID defID = initInfo.defID;
		entityInst.init(instID, defID, positionID, bboxID);

		WorldDouble3 &entityPosition = this->positions.get(positionID);
		entityPosition = initInfo.feetPosition;

		BoundingBox3D &entityBBox = this->boundingBoxes.get(bboxID);
		entityBBox.init(initInfo.bboxMin, initInfo.bboxMax);

		if (!this->animInsts.tryAlloc(&entityInst.animInstID))
		{
			DebugLogError("Couldn't allocate EntityAnimationInstanceID.");
		}

		const EntityAnimationDefinition &animDef = entityDef.animDef;
		EntityAnimationInstance &animInst = this->animInsts.get(entityInst.animInstID);
		for (int animDefStateIndex = 0; animDefStateIndex < animDef.stateCount; animDefStateIndex++)
		{
			const EntityAnimationDefinitionState &animDefState = animDef.states[animDefStateIndex];
			animInst.addState(animDefState.seconds, animDefState.isLooping);
		}
		
		animInst.setStateIndex(initInfo.initialAnimStateIndex);

		if (!TryCreatePhysicsCollider(entityPosition, initInfo.animMaxHeight, initInfo.isSensorCollider, physicsSystem, &entityInst.physicsBodyID))
		{
			DebugLogError("Couldn't allocate entity Jolt physics body.");
		}

		if (initInfo.direction.has_value())
		{
			if (!this->directions.tryAlloc(&entityInst.directionID))
			{
				DebugLogError("Couldn't allocate EntityDirectionID.");
			}

			const Double2 &direction = *initInfo.direction;
			this->directions.get(entityInst.directionID) = direction;
		}

		if (initInfo.citizenDirectionIndex.has_value())
		{
			if (!this->citizenDirectionIndices.tryAlloc(&entityInst.citizenDirectionIndexID))
			{
				DebugLogError("Couldn't allocate EntityCitizenDirectionIndexID.");
			}

			const uint8_t citizenDirectionIndex = *initInfo.citizenDirectionIndex;
			this->citizenDirectionIndices.get(entityInst.citizenDirectionIndexID) = citizenDirectionIndex;
		}

		if (initInfo.citizenColorSeed.has_value())
		{
			if (!this->paletteIndices.tryAlloc(&entityInst.paletteIndicesInstID))
			{
				DebugLogError("Couldn't allocate EntityPaletteIndicesInstanceID.");
			}

			DebugAssert(initInfo.raceID.has_value());
			const uint16_t citizenColorSeed = *initInfo.citizenColorSeed;
			PaletteIndices &paletteIndices = this->paletteIndices.get(entityInst.paletteIndicesInstID);
			paletteIndices = ArenaAnimUtils::transformCitizenColors(*initInfo.raceID, citizenColorSeed, binaryAssetLibrary.getExeData());
		}

		if (initInfo.hasInventory)
		{
			if (!this->itemInventories.tryAlloc(&entityInst.itemInventoryInstID))
			{
				DebugCrash("Couldn't allocate EntityItemInventoryInstanceID.");
			}
		}

		if (initInfo.hasCreatureSound)
		{
			if (!this->creatureSoundInsts.tryAlloc(&entityInst.creatureSoundInstID))
			{
				DebugCrash("Couldn't allocate EntityCreatureSoundInstanceID.");
			}

			double &secondsTillNextCreatureSound = this->creatureSoundInsts.get(entityInst.creatureSoundInstID);
			secondsTillNextCreatureSound = EntityUtils::nextCreatureSoundWaitSeconds(random);
		}
	};

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
		
		const std::optional<int> initialAnimStateIndex = animDef.tryGetStateIndex(initialAnimStateName);
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
			const VoxelShapeDefID voxelShapeDefID = voxelChunk.getShapeDefID(voxel.x, voxel.y, voxel.z);
			const VoxelShapeDefinition &voxelShapeDef = voxelChunk.getShapeDef(voxelShapeDefID);
			const double feetY = ceilingScale + entityDefYOffset + GetElevatedPlatformHeight(voxelShapeDef, ceilingScale);

			double animMaxWidth, animMaxHeight;
			EntityUtils::getAnimationMaxDims(animDef, &animMaxWidth, &animMaxHeight);
			const double halfAnimMaxWidth = animMaxWidth * 0.50;

			EntityInitInfo initInfo;
			initInfo.defID = *entityDefID;
			initInfo.feetPosition = WorldDouble3(worldPositionXZ.x, feetY, worldPositionXZ.y);

			// Bounding box is centered on the entity in model space.
			initInfo.bboxMin = WorldDouble3(-halfAnimMaxWidth, 0.0, -halfAnimMaxWidth);
			initInfo.bboxMax = WorldDouble3(halfAnimMaxWidth, animMaxHeight, halfAnimMaxWidth);

			initInfo.animMaxHeight = animMaxHeight;
			initInfo.initialAnimStateIndex = *initialAnimStateIndex;
			initInfo.isSensorCollider = !EntityUtils::hasCollision(entityDef);

			if (isDynamicEntity)
			{
				initInfo.direction = CardinalDirection::North;
				initInfo.hasInventory = (entityDefType == EntityDefinitionType::Enemy) || (entityDefType == EntityDefinitionType::Container);
				initInfo.hasCreatureSound = (entityDefType == EntityDefinitionType::Enemy) && (entityDef.enemy.type == EnemyEntityDefinitionType::Creature);
			}

			EntityInstanceID entityInstID = this->spawnEntity();
			EntityInstance &entityInst = this->entities.get(entityInstID);
			initializeEntity(entityInst, entityInstID, entityDef, initInfo);
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
				const VoxelTraitsDefID voxelTraitsDefID = voxelChunk.getTraitsDefID(spawnVoxel.x, 1, spawnVoxel.y);
				const VoxelTraitsDefID groundVoxelTraitsDefID = voxelChunk.getTraitsDefID(spawnVoxel.x, 0, spawnVoxel.y);
				const VoxelTraitsDefinition &voxelTraitsDef = voxelChunk.getTraitsDef(voxelTraitsDefID);
				const VoxelTraitsDefinition &groundVoxelTraitsDef = voxelChunk.getTraitsDef(groundVoxelTraitsDefID);
				const bool isValidSpawnVoxel = (voxelTraitsDef.type == ArenaTypes::VoxelType::None) && (groundVoxelTraitsDef.type == ArenaTypes::VoxelType::Floor);				
				if (isValidSpawnVoxel)
				{
					return spawnVoxel;
				}
			}

			return std::nullopt;
		};

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
			DebugAssertIndex(citizenCountsToSpawn, citizenGenderIndex);
			const int citizensToSpawn = citizenCountsToSpawn[citizenGenderIndex];
			const EntityDefID citizenEntityDefID = citizenDefIDs[citizenGenderIndex];
			const EntityDefinition &citizenDef = *citizenDefs[citizenGenderIndex];			
			const EntityAnimationDefinition &citizenAnimDef = citizenDef.animDef;

			const std::optional<int> initialCitizenAnimStateIndex = citizenAnimDef.tryGetStateIndex(citizenAnimDef.initialStateName);
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

				double animMaxWidth, animMaxHeight;
				EntityUtils::getAnimationMaxDims(citizenDef.animDef, &animMaxWidth, &animMaxHeight);
				const double halfAnimMaxWidth = animMaxWidth * 0.50;

				// Bounding box is centered on the entity in model space.
				citizenInitInfo.bboxMin = WorldDouble3(-halfAnimMaxWidth, 0.0, -halfAnimMaxWidth);
				citizenInitInfo.bboxMax = WorldDouble3(halfAnimMaxWidth, animMaxHeight, halfAnimMaxWidth);

				citizenInitInfo.animMaxHeight = animMaxHeight;
				citizenInitInfo.initialAnimStateIndex = *initialCitizenAnimStateIndex;
				citizenInitInfo.isSensorCollider = true;
				citizenInitInfo.citizenDirectionIndex = CitizenUtils::getRandomCitizenDirectionIndex(random);
				citizenInitInfo.direction = CitizenUtils::getCitizenDirectionByIndex(*citizenInitInfo.citizenDirectionIndex);
				citizenInitInfo.citizenColorSeed = static_cast<uint16_t>(random.next() % std::numeric_limits<uint16_t>::max());
				citizenInitInfo.raceID = citizenRaceID;
				citizenInitInfo.hasInventory = false;
				citizenInitInfo.hasCreatureSound = false;

				EntityInstanceID entityInstID = this->spawnEntity();
				EntityInstance &entityInst = this->entities.get(entityInstID);
				initializeEntity(entityInst, entityInstID, citizenDef, citizenInitInfo);
				entityChunk.entityIDs.emplace_back(entityInstID);
			}
		}
	}
}

void EntityChunkManager::populateChunk(EntityChunk &entityChunk, const VoxelChunk &voxelChunk,
	const LevelDefinition &levelDef, const LevelInfoDefinition &levelInfoDef, const MapSubDefinition &mapSubDef, 
	const EntityGeneration::EntityGenInfo &entityGenInfo, const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
	double ceilingScale, Random &random, const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	JPH::PhysicsSystem &physicsSystem, TextureManager &textureManager, Renderer &renderer)
{
	const ChunkInt2 &chunkPos = entityChunk.getPosition();
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
				citizenGenInfo, random, entityDefLibrary, binaryAssetLibrary, physicsSystem, textureManager, renderer);
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
				citizenGenInfo, random, entityDefLibrary, binaryAssetLibrary, physicsSystem, textureManager, renderer);
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
			citizenGenInfo, random, entityDefLibrary, binaryAssetLibrary, physicsSystem, textureManager, renderer);
	}
}

void EntityChunkManager::updateCitizenStates(double dt, EntityChunk &entityChunk, const CoordDouble2 &playerCoordXZ,
	bool isPlayerMoving, bool isPlayerWeaponSheathed, Random &random, JPH::PhysicsSystem &physicsSystem, const VoxelChunkManager &voxelChunkManager)
{
	JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();

	for (int i = static_cast<int>(entityChunk.entityIDs.size()) - 1; i >= 0; i--)
	{
		const EntityInstanceID entityInstID = entityChunk.entityIDs[i];
		const EntityInstance &entityInst = this->entities.get(entityInstID);
		if (!entityInst.isCitizen())
		{
			continue;
		}

		WorldDouble3 &entityPosition = this->positions.get(entityInst.positionID);
		const CoordDouble3 entityCoord = VoxelUtils::worldPointToCoord(entityPosition);
		const CoordDouble2 entityCoordXZ(entityCoord.chunk, VoxelDouble2(entityCoord.point.x, entityCoord.point.z));
		const ChunkInt2 prevEntityChunkPos = entityCoordXZ.chunk;
		ChunkInt2 curEntityChunkPos = prevEntityChunkPos; // Potentially updated by entity movement.
		const VoxelDouble2 dirToPlayer = playerCoordXZ - entityCoordXZ;
		const double distToPlayerSqr = dirToPlayer.lengthSquared();

		const EntityDefinition &entityDef = this->getEntityDef(entityInst.defID);
		const EntityAnimationDefinition &animDef = entityDef.animDef;

		const std::optional<int> idleStateIndex = animDef.tryGetStateIndex(EntityAnimationUtils::STATE_IDLE.c_str());
		if (!idleStateIndex.has_value())
		{
			DebugCrash("Couldn't get citizen idle state index.");
		}

		const std::optional<int> walkStateIndex = animDef.tryGetStateIndex(EntityAnimationUtils::STATE_WALK.c_str());
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
			auto getVoxelAtDistance = [&entityCoordXZ](const VoxelDouble2 &checkDist) -> CoordInt2
			{
				const CoordDouble2 pos = entityCoordXZ + checkDist;
				return CoordInt2(pos.chunk, VoxelUtils::pointToVoxel(pos.point));
			};

			const CoordInt2 curVoxel(entityCoordXZ.chunk, VoxelUtils::pointToVoxel(entityCoordXZ.point));
			const CoordInt2 nextVoxel = getVoxelAtDistance(entityDir * 0.50);

			if (nextVoxel != curVoxel)
			{
				auto isSuitableVoxel = [&voxelChunkManager](const CoordInt2 &coord)
				{
					const VoxelChunk *voxelChunk = voxelChunkManager.tryGetChunkAtPosition(coord.chunk);

					auto isValidVoxel = [voxelChunk]()
					{
						return voxelChunk != nullptr;
					};

					auto isPassableVoxel = [&coord, voxelChunk]()
					{
						const VoxelInt3 voxel(coord.voxel.x, 1, coord.voxel.y);
						const VoxelTraitsDefID voxelTraitsDefID = voxelChunk->getTraitsDefID(voxel.x, voxel.y, voxel.z);
						const VoxelTraitsDefinition &voxelTraitsDef = voxelChunk->getTraitsDef(voxelTraitsDefID);
						return voxelTraitsDef.type == ArenaTypes::VoxelType::None;
					};

					auto isWalkableVoxel = [&coord, voxelChunk]()
					{
						const VoxelInt3 voxel(coord.voxel.x, 0, coord.voxel.y);
						const VoxelTraitsDefID voxelTraitsDefID = voxelChunk->getTraitsDefID(voxel.x, voxel.y, voxel.z);
						const VoxelTraitsDefinition &voxelTraitsDef = voxelChunk->getTraitsDef(voxelTraitsDefID);
						return voxelTraitsDef.type == ArenaTypes::VoxelType::Floor;
					};

					return isValidVoxel() && isPassableVoxel() && isWalkableVoxel();
				};

				if (!isSuitableVoxel(nextVoxel))
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
							const WorldDouble2 &direction = CitizenUtils::getCitizenDirectionByIndex(dirIndex);
							const CoordInt2 voxel = getVoxelAtDistance(direction * 0.50);
							if (isSuitableVoxel(voxel))
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
			const CoordDouble2 curEntityCoordXZ = ChunkUtils::recalculateCoord(entityCoordXZ.chunk, entityCoordXZ.point + (entityVelocity * dt));
			const WorldDouble2 curEntityPositionXZ = VoxelUtils::coordToWorldPoint(curEntityCoordXZ);
			entityPosition.x = curEntityPositionXZ.x;
			entityPosition.z = curEntityPositionXZ.y;			
			curEntityChunkPos = curEntityCoordXZ.chunk;

			const JPH::BodyID &physicsBodyID = entityInst.physicsBodyID;
			DebugAssert(!physicsBodyID.IsInvalid());

			const JPH::RVec3 oldBodyPosition = bodyInterface.GetPosition(physicsBodyID);
			const WorldDouble2 newEntityWorldPointXZ = VoxelUtils::coordToWorldPoint(curEntityCoordXZ);
			const JPH::RVec3 newBodyPosition(
				static_cast<float>(newEntityWorldPointXZ.x),
				static_cast<float>(oldBodyPosition.GetY()),
				static_cast<float>(newEntityWorldPointXZ.y));
			bodyInterface.SetPosition(physicsBodyID, newBodyPosition, JPH::EActivation::Activate);
		}

		// Transfer ownership of the entity ID to a new chunk if needed.
		if (curEntityChunkPos != prevEntityChunkPos)
		{
			EntityChunk &curEntityChunk = this->getChunkAtPosition(curEntityChunkPos);
			entityChunk.entityIDs.erase(entityChunk.entityIDs.begin() + i);
			curEntityChunk.entityIDs.emplace_back(entityInstID);

			EntityTransferResult transferResult;
			transferResult.id = entityInstID;
			transferResult.oldChunkPos = prevEntityChunkPos;
			transferResult.newChunkPos = curEntityChunkPos;
			this->transferResults.emplace_back(std::move(transferResult));
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

const VoxelDouble2 &EntityChunkManager::getEntityDirection(EntityDirectionID id) const
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

int8_t EntityChunkManager::getEntityCitizenDirectionIndex(EntityCitizenDirectionIndexID id) const
{
	return this->citizenDirectionIndices.get(id);
}

const PaletteIndices &EntityChunkManager::getEntityPaletteIndices(EntityPaletteIndicesInstanceID id) const
{
	return this->paletteIndices.get(id);
}

ItemInventory &EntityChunkManager::getEntityItemInventory(EntityItemInventoryInstanceID id)
{
	return this->itemInventories.get(id);
}

EntityInstanceID EntityChunkManager::getEntityFromPhysicsBodyID(JPH::BodyID bodyID) const
{
	if (bodyID.IsInvalid())
	{
		return -1;
	}

	// @todo: probably want a smarter lookup than this
	for (int i = 0; i < this->entities.getTotalCount(); i++)
	{
		const EntityInstance *entityInst = this->entities.tryGet(i);
		if (entityInst != nullptr && entityInst->physicsBodyID == bodyID)
		{
			return entityInst->instanceID;
		}
	}

	return -1;
}

int EntityChunkManager::getCountInChunkWithDirection(const ChunkInt2 &chunkPos) const
{
	int count = 0;
	const std::optional<int> chunkIndex = this->tryGetChunkIndex(chunkPos);
	if (!chunkIndex.has_value())
	{
		DebugLogWarning("Missing chunk (" + chunkPos.toString() + ") for counting entities with direction.");
		return 0;
	}

	const EntityChunk &chunk = this->getChunkAtIndex(*chunkIndex);
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
	int count = 0;
	const std::optional<int> chunkIndex = this->tryGetChunkIndex(chunkPos);
	if (!chunkIndex.has_value())
	{
		DebugLogWarning("Missing chunk (" + chunkPos.toString() + ") for counting entities with creature sound.");
		return 0;
	}

	const EntityChunk &chunk = this->getChunkAtIndex(*chunkIndex);
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
	int count = 0;
	const std::optional<int> chunkIndex = this->tryGetChunkIndex(chunkPos);
	if (!chunkIndex.has_value())
	{
		DebugLogWarning("Missing chunk (" + chunkPos.toString() + ") for counting entities with citizen direction.");
		return 0;
	}

	const EntityChunk &chunk = this->getChunkAtIndex(*chunkIndex);
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

BufferView<const EntityInstanceID> EntityChunkManager::getQueuedDestroyEntityIDs() const
{
	return this->destroyedEntityIDs;
}

BufferView<const EntityTransferResult> EntityChunkManager::getEntityTransferResults() const
{
	return this->transferResults;
}

void EntityChunkManager::getEntityObservedResult(EntityInstanceID id, const WorldDouble3 &eyePosition, EntityObservedResult &result) const
{
	const EntityInstance &entityInst = this->entities.get(id);
	const EntityDefinition &entityDef = this->getEntityDef(entityInst.defID);
	const EntityAnimationDefinition &animDef = entityDef.animDef;
	const EntityAnimationInstance &animInst = this->animInsts.get(entityInst.animInstID);

	const WorldDouble2 eyePositionXZ(eyePosition.x, eyePosition.z);
	const WorldDouble3 entityPosition = this->positions.get(entityInst.positionID);
	const WorldDouble2 entityPositionXZ(entityPosition.x, entityPosition.z);
	const bool isDynamic = entityInst.isDynamic();

	const int stateIndex = animInst.currentStateIndex;
	DebugAssert(stateIndex >= 0);
	DebugAssert(stateIndex < animDef.stateCount);
	const EntityAnimationDefinitionState &animDefState = animDef.states[stateIndex];
	const int angleCount = animDefState.keyframeListCount;

	// Get animation angle based on relative facing to camera. Static entities always face the camera.
	Radians animAngle = 0.0;
	if (isDynamic)
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
	DebugAssert(animDefKeyframeListIndex >= 0);
	DebugAssert(animDefKeyframeListIndex < animDef.keyframeListCount);
	const EntityAnimationDefinitionKeyframeList &animDefKeyframeList = animDef.keyframeLists[animDefKeyframeListIndex];

	// Get current keyframe.
	const int keyframeCount = animDefKeyframeList.keyframeCount;
	const double keyframeCountReal = static_cast<double>(keyframeCount);
	const int unclampedKeyframeIndex = static_cast<int>(keyframeCountReal * animInst.progressPercent);
	const int keyframeIndex = std::clamp(unclampedKeyframeIndex, 0, keyframeCount - 1);

	const int linearizedKeyframeIndex = animDef.getLinearizedKeyframeIndex(stateIndex, angleIndex, keyframeIndex);
	result.init(id, stateIndex, angleIndex, keyframeIndex, linearizedKeyframeIndex);
}

void EntityChunkManager::updateCreatureSounds(double dt, EntityChunk &entityChunk, const WorldDouble3 &playerPosition,
	Random &random, AudioManager &audioManager)
{
	const int entityCount = static_cast<int>(entityChunk.entityIDs.size());
	for (int i = 0; i < entityCount; i++)
	{
		const EntityInstanceID instID = entityChunk.entityIDs[i];
		EntityInstance &entityInst = this->entities.get(instID);
		if (entityInst.creatureSoundInstID >= 0)
		{
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
	for (const VoxelFadeAnimationInstance &fadeAnimInst : voxelChunk.getFadeAnimInsts())
	{
		if (fadeAnimInst.isDoneFading())
		{
			for (int i = static_cast<int>(entityChunk.entityIDs.size()) - 1; i >= 0; i--)
			{
				const EntityInstanceID entityInstID = entityChunk.entityIDs[i];
				const EntityInstance &entityInst = this->entities.get(entityInstID);
				WorldDouble3 &entityPosition = this->positions.get(entityInst.positionID);
				const CoordDouble3 entityCoord = VoxelUtils::worldPointToCoord(entityPosition);
				const CoordInt3 entityVoxelCoord(entityCoord.chunk, VoxelUtils::pointToVoxel(entityCoord.point, ceilingScale));
				const VoxelInt3 entityVoxel = entityVoxelCoord.voxel;

				const bool matchesFadedVoxel = (entityVoxel.x == fadeAnimInst.x) && (entityVoxel.z == fadeAnimInst.z);
				
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

void EntityChunkManager::update(double dt, BufferView<const ChunkInt2> activeChunkPositions,
	BufferView<const ChunkInt2> newChunkPositions, BufferView<const ChunkInt2> freedChunkPositions,
	const Player &player, const LevelDefinition *activeLevelDef, const LevelInfoDefinition *activeLevelInfoDef,
	const MapSubDefinition &mapSubDef, BufferView<const LevelDefinition> levelDefs,
	BufferView<const int> levelInfoDefIndices, BufferView<const LevelInfoDefinition> levelInfoDefs,
	const EntityGeneration::EntityGenInfo &entityGenInfo, const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
	double ceilingScale, Random &random, const VoxelChunkManager &voxelChunkManager, AudioManager &audioManager,
	JPH::PhysicsSystem &physicsSystem, TextureManager &textureManager, Renderer &renderer)
{
	const EntityDefinitionLibrary &entityDefLibrary = EntityDefinitionLibrary::getInstance();
	const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();

	for (const ChunkInt2 &chunkPos : freedChunkPositions)
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
	for (const ChunkInt2 &chunkPos : newChunkPositions)
	{
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		const int spawnIndex = this->spawnChunk();
		EntityChunk &entityChunk = this->getChunkAtIndex(spawnIndex);
		entityChunk.init(chunkPos, voxelChunk.getHeight());

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
			ceilingScale, random, entityDefLibrary, binaryAssetLibrary, physicsSystem, textureManager, renderer);
	}

	// Free any unneeded chunks for memory savings in case the chunk distance was once large
	// and is now small. This is significant even for chunk distance 2->1, or 25->9 chunks.
	this->chunkPool.clear();

	const WorldDouble3 playerPosition = player.getEyePosition();
	const CoordDouble3 playerCoord = player.getEyeCoord();
	const CoordDouble2 playerCoordXZ(playerCoord.chunk, VoxelDouble2(playerCoord.point.x, playerCoord.point.z));
	const bool isPlayerMoving = player.isMoving();

	const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
	const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(player.weaponAnimDefID);
	const WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
	const WeaponAnimationDefinitionState &weaponAnimDefState = weaponAnimDef.states[weaponAnimInst.currentStateIndex];
	const bool isPlayerWeaponSheathed = WeaponAnimationUtils::isSheathed(weaponAnimDefState);

	for (const ChunkInt2 &chunkPos : activeChunkPositions)
	{
		const int chunkIndex = this->getChunkIndex(chunkPos);
		EntityChunk &entityChunk = this->getChunkAtIndex(chunkIndex);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);

		// @todo: simulate/animate AI
		this->updateCitizenStates(dt, entityChunk, playerCoordXZ, isPlayerMoving, isPlayerWeaponSheathed, random, physicsSystem, voxelChunkManager);

		for (const EntityInstanceID entityInstID : entityChunk.entityIDs)
		{
			const EntityInstance &entityInst = this->entities.get(entityInstID);
			EntityAnimationInstance &animInst = this->animInsts.get(entityInst.animInstID);
			animInst.update(dt);
		}

		this->updateCreatureSounds(dt, entityChunk, playerPosition, random, audioManager);
		this->updateFadedElevatedPlatforms(entityChunk, voxelChunk, ceilingScale, physicsSystem);
	}
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
	if (notifyChunk)
	{
		const EntityInstance &entityInst = this->entities.get(entityInstID);
		const WorldDouble3 entityPosition = this->positions.get(entityInst.positionID);
		const CoordDouble3 entityCoord = VoxelUtils::worldPointToCoord(entityPosition);
		chunkToNotify = &entityCoord.chunk;
	}
	
	this->queueEntityDestroy(entityInstID, chunkToNotify);
}

void EntityChunkManager::cleanUp(JPH::PhysicsSystem &physicsSystem)
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

		if (entityInst.creatureSoundInstID >= 0)
		{
			this->creatureSoundInsts.free(entityInst.creatureSoundInstID);
		}

		if (entityInst.citizenDirectionIndexID >= 0)
		{
			this->citizenDirectionIndices.free(entityInst.citizenDirectionIndexID);
		}

		if (entityInst.paletteIndicesInstID >= 0)
		{
			this->paletteIndices.free(entityInst.paletteIndicesInstID);
		}

		if (entityInst.itemInventoryInstID >= 0)
		{
			this->itemInventories.free(entityInst.itemInventoryInstID);
		}

		const JPH::BodyID physicsBodyID = entityInst.physicsBodyID;
		if (!physicsBodyID.IsInvalid())
		{
			bodyInterface.RemoveBody(physicsBodyID);
			bodyInterface.DestroyBody(physicsBodyID);
		}
		
		this->entities.free(entityInstID);
	}

	this->destroyedEntityIDs.clear();
	this->transferResults.clear();
}

void EntityChunkManager::clear(JPH::PhysicsSystem &physicsSystem)
{
	for (ChunkPtr &chunkPtr : this->activeChunks)
	{
		for (const EntityInstanceID entityInstID : chunkPtr->entityIDs)
		{
			this->queueEntityDestroy(entityInstID, false);
		}
	}

	this->cleanUp(physicsSystem);
	this->recycleAllChunks();
}
