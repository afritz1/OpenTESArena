#include "DynamicEntity.h"
#include "EntityChunkManager.h"
#include "EntityDefinitionLibrary.h"
#include "EntityType.h"
#include "../Audio/AudioManager.h"
#include "../Game/CardinalDirection.h"
#include "../Math/Random.h"
#include "../Voxels/VoxelChunk.h"
#include "../Voxels/VoxelChunkManager.h"
#include "../World/LevelDefinition.h"
#include "../World/LevelInfoDefinition.h"
#include "../World/MapDefinition.h"
#include "../World/MapType.h"

#include "components/utilities/String.h"

const EntityDefinition &EntityChunkManager::getEntityDef(EntityDefID defID, const EntityDefinitionLibrary &defLibrary) const
{
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

void EntityChunkManager::populateChunkEntities(EntityChunk &entityChunk, const VoxelChunk &chunk,
	const LevelDefinition &levelDefinition, const LevelInfoDefinition &levelInfoDefinition, const LevelInt2 &levelOffset,
	const EntityGeneration::EntityGenInfo &entityGenInfo, const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
	Random &random, const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	TextureManager &textureManager)
{
	SNInt startX, endX;
	int startY, endY;
	WEInt startZ, endZ;
	ChunkUtils::GetWritingRanges(levelOffset, levelDefinition.getWidth(), levelDefinition.getHeight(),
		levelDefinition.getDepth(), &startX, &startY, &startZ, &endX, &endY, &endZ);

	const double ceilingScale = levelInfoDefinition.getCeilingScale();

	for (int i = 0; i < levelDefinition.getEntityPlacementDefCount(); i++)
	{
		const LevelDefinition::EntityPlacementDef &placementDef = levelDefinition.getEntityPlacementDef(i);
		const LevelDefinition::EntityDefID levelEntityDefID = placementDef.id;
		const EntityDefinition &entityDef = levelInfoDefinition.getEntityDef(levelEntityDefID);
		const EntityDefinition::Type entityDefType = entityDef.getType();
		const EntityType entityType = EntityUtils::getEntityTypeFromDefType(entityDefType);
		const EntityAnimationDefinition &animDef = entityDef.getAnimDef();

		std::optional<EntityDefID> entityDefID; // Global entity def ID (shared across all active chunks).
		for (const LevelDouble3 &position : placementDef.positions)
		{
			const LevelInt3 voxelPosition = VoxelUtils::pointToVoxel(position, ceilingScale);
			if (ChunkUtils::IsInWritingRange(voxelPosition, startX, endX, startY, endY, startZ, endZ))
			{
				if (!entityDefID.has_value())
				{
					entityDefID = this->getOrAddEntityDefID(entityDef, entityDefLibrary);
				}

				const VoxelDouble3 point = ChunkUtils::MakeChunkPointFromLevel(position, startX, startY, startZ);
				EntityInstanceID entityInstID = this->spawnEntity();
				EntityInstance &entityInst = this->entities.get(entityInstID);

				EntityPositionID positionID;
				if (!this->positions.tryAlloc(&positionID))
				{
					DebugCrash("Couldn't allocate EntityPositionID.");
				}

				entityInst.init(entityInstID, *entityDefID, positionID);

				EntityAnimationInstance animInst;
				animInst.init(animDef);

				const std::string &defaultStateName = EntityGeneration::getDefaultAnimationStateName(entityDef, entityGenInfo);
				const std::optional<int> defaultStateIndex = animDef.tryGetStateIndex(defaultStateName.c_str());
				if (!defaultStateIndex.has_value())
				{
					DebugLogWarning("Couldn't get default state index for entity.");
					continue;
				}

				animInst.setStateIndex(*defaultStateIndex);

				CoordDouble2 &entityCoord = this->positions.get(positionID);
				entityCoord.chunk = chunk.getPosition();
				entityCoord.point = VoxelDouble2(point.x, point.z);

				if (entityType == EntityType::Dynamic) // Dynamic entities have a direction.
				{
					if (!this->directions.tryAlloc(&entityInst.directionID))
					{
						DebugCrash("Couldn't allocate EntityDirectionID.");
					}

					VoxelDouble2 &entityDir = this->directions.get(entityInst.directionID);
					entityDir = CardinalDirection::North;

					if (entityDefType == EntityDefinition::Type::Enemy)
					{
						if (!this->creatureSoundInsts.tryAlloc(&entityInst.creatureSoundInstID))
						{
							DebugCrash("Couldn't allocate EntityCreatureSoundInstanceID.");
						}

						double &secondsTillCreatureSound = this->creatureSoundInsts.get(entityInst.creatureSoundInstID);
						secondsTillCreatureSound = DynamicEntity::nextCreatureSoundWaitTime(random);
					}
				}

				entityChunk.entityIDs.emplace_back(entityInstID);
			}
		}
	}

	if (citizenGenInfo.has_value())
	{
		DebugLogError("Not implemented: citizen spawning");

		// Spawn citizens if the total active limit has not been reached.
		/*const int currentCitizenCount = CitizenUtils::getCitizenCount(entityManager);
		const int remainingCitizensToSpawn = std::min(
			CitizenUtils::MAX_ACTIVE_CITIZENS - currentCitizenCount, CitizenUtils::CITIZENS_PER_CHUNK);

		for (int i = 0; i < remainingCitizensToSpawn; i++)
		{
			if (!CitizenUtils::trySpawnCitizenInChunk(chunk, *citizenGenInfo, random, binaryAssetLibrary, textureManager, entityManager))
			{
				DebugLogWarning("Couldn't spawn citizen in chunk \"" + chunk.getPosition().toString() + "\".");
			}
		}*/
	}
}

void EntityChunkManager::populateChunk(EntityChunk &entityChunk, const VoxelChunk &voxelChunk,
	const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
	const EntityGeneration::EntityGenInfo &entityGenInfo, const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
	double ceilingScale, Random &random, const EntityDefinitionLibrary &entityDefLibrary,
	const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager, Renderer &renderer)
{
	const ChunkInt2 &chunkPos = entityChunk.getPosition();

	// Populate all or part of the chunk from a level definition depending on the map type.
	const MapType mapType = mapDefinition.getMapType();
	if (mapType == MapType::Interior)
	{
		DebugAssert(activeLevelIndex.has_value());
		const LevelDefinition &levelDefinition = mapDefinition.getLevel(*activeLevelIndex);
		const LevelInfoDefinition &levelInfoDefinition = mapDefinition.getLevelInfoForLevel(*activeLevelIndex);

		if (ChunkUtils::touchesLevelDimensions(chunkPos, levelDefinition.getWidth(), levelDefinition.getDepth()))
		{
			// Populate chunk from the part of the level it overlaps.
			const LevelInt2 levelOffset = chunkPos * ChunkUtils::CHUNK_DIM;
			DebugAssert(!citizenGenInfo.has_value());
			this->populateChunkEntities(entityChunk, voxelChunk, levelDefinition, levelInfoDefinition, levelOffset, entityGenInfo,
				citizenGenInfo, random, entityDefLibrary, binaryAssetLibrary, textureManager);
		}
	}
	else if (mapType == MapType::City)
	{
		DebugAssert(activeLevelIndex.has_value() && (*activeLevelIndex == 0));
		const LevelDefinition &levelDefinition = mapDefinition.getLevel(0);
		const LevelInfoDefinition &levelInfoDefinition = mapDefinition.getLevelInfoForLevel(0);

		if (ChunkUtils::touchesLevelDimensions(chunkPos, levelDefinition.getWidth(), levelDefinition.getDepth()))
		{
			// Populate chunk from the part of the level it overlaps.
			const LevelInt2 levelOffset = chunkPos * ChunkUtils::CHUNK_DIM;
			DebugAssert(citizenGenInfo.has_value());
			this->populateChunkEntities(entityChunk, voxelChunk, levelDefinition, levelInfoDefinition, levelOffset, entityGenInfo,
				citizenGenInfo, random, entityDefLibrary, binaryAssetLibrary, textureManager);
		}
	}
	else if (mapType == MapType::Wilderness)
	{
		// The wilderness doesn't have an active level index since it's always just the one level.
		DebugAssert(!activeLevelIndex.has_value() || (*activeLevelIndex == 0));

		const MapDefinition::Wild &mapDefWild = mapDefinition.getWild();
		const int levelDefIndex = mapDefWild.getLevelDefIndex(chunkPos);
		const LevelDefinition &levelDefinition = mapDefinition.getLevel(levelDefIndex);
		const LevelInfoDefinition &levelInfoDefinition = mapDefinition.getLevelInfoForLevel(levelDefIndex);

		// Copy level definition directly into chunk.
		DebugAssert(levelDefinition.getWidth() == Chunk::WIDTH);
		DebugAssert(levelDefinition.getDepth() == Chunk::DEPTH);
		const LevelInt2 levelOffset = LevelInt2::Zero;

		DebugAssert(citizenGenInfo.has_value());
		this->populateChunkEntities(entityChunk, voxelChunk, levelDefinition, levelInfoDefinition, levelOffset, entityGenInfo,
			citizenGenInfo, random, entityDefLibrary, binaryAssetLibrary, textureManager);
	}
}

std::string EntityChunkManager::getCreatureSoundFilename(const EntityDefID defID, const EntityDefinitionLibrary &entityDefLibrary) const
{
	const EntityDefinition &entityDef = this->getEntityDef(defID, entityDefLibrary);
	if (entityDef.getType() != EntityDefinition::Type::Enemy)
	{
		return std::string();
	}

	const auto &enemyDef = entityDef.getEnemy();
	if (enemyDef.getType() != EntityDefinition::EnemyDefinition::Type::Creature)
	{
		return std::string();
	}

	const auto &creatureDef = enemyDef.getCreature();
	const std::string_view creatureSoundName = creatureDef.soundName;
	return String::toUppercase(std::string(creatureSoundName));
}

void EntityChunkManager::updateCreatureSounds(double dt, EntityChunk &entityChunk, const CoordDouble3 &playerCoord,
	double ceilingScale, Random &random, const EntityDefinitionLibrary &entityDefLibrary, AudioManager &audioManager)
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
				const CoordDouble2 &entityCoord = this->positions.get(entityInst.positionID);
				if (EntityUtils::withinHearingDistance(playerCoord, entityCoord, ceilingScale))
				{
					// @todo: store some kind of sound def ID w/ the secondsTillCreatureSound instead of generating the sound filename here.
					const std::string creatureSoundFilename = this->getCreatureSoundFilename(entityInst.defID, entityDefLibrary);
					if (creatureSoundFilename.empty())
					{
						continue;
					}

					// Center the sound inside the creature.
					const CoordDouble3 soundCoord(
						entityCoord.chunk,
						VoxelDouble3(entityCoord.point.x, ceilingScale * 1.50, entityCoord.point.y));
					const WorldDouble3 absoluteSoundPosition = VoxelUtils::coordToWorldPoint(soundCoord);
					audioManager.playSound(creatureSoundFilename, absoluteSoundPosition);

					secondsTillCreatureSound = DynamicEntity::nextCreatureSoundWaitTime(random);
				}
			}
		}
	}
}

void EntityChunkManager::update(double dt, const BufferView<const ChunkInt2> &activeChunkPositions,
	const BufferView<const ChunkInt2> &newChunkPositions, const BufferView<const ChunkInt2> &freedChunkPositions,
	const CoordDouble3 &playerCoord, const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
	const EntityGeneration::EntityGenInfo &entityGenInfo, const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
	double ceilingScale, Random &random, const VoxelChunkManager &voxelChunkManager, const EntityDefinitionLibrary &entityDefLibrary,
	const BinaryAssetLibrary &binaryAssetLibrary, AudioManager &audioManager, TextureManager &textureManager, Renderer &renderer)
{
	for (int i = 0; i < freedChunkPositions.getCount(); i++)
	{
		const ChunkInt2 &chunkPos = freedChunkPositions.get(i);
		const int chunkIndex = this->getChunkIndex(chunkPos);
		this->recycleChunk(chunkIndex);
	}

	for (int i = 0; i < newChunkPositions.getCount(); i++)
	{
		const ChunkInt2 &chunkPos = newChunkPositions.get(i);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);

		const int spawnIndex = this->spawnChunk();
		EntityChunk &entityChunk = this->getChunkAtIndex(spawnIndex);
		entityChunk.init(chunkPos, voxelChunk.getHeight());

		this->populateChunk(entityChunk, voxelChunk, activeLevelIndex, mapDefinition, entityGenInfo, citizenGenInfo,
			ceilingScale, random, entityDefLibrary, binaryAssetLibrary, textureManager, renderer);
	}

	// Free any unneeded chunks for memory savings in case the chunk distance was once large
	// and is now small. This is significant even for chunk distance 2->1, or 25->9 chunks.
	this->chunkPool.clear();

	for (int i = 0; i < activeChunkPositions.getCount(); i++)
	{
		const ChunkInt2 &chunkPos = activeChunkPositions.get(i);
		const int chunkIndex = this->getChunkIndex(chunkPos);
		EntityChunk &entityChunk = this->getChunkAtIndex(chunkIndex);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);

		// @todo: simulate/animate AI

		this->updateCreatureSounds(dt, entityChunk, playerCoord, ceilingScale, random, entityDefLibrary, audioManager);

		// @todo: citizen spawning and management by player distance

		// @todo: rebuild entity chunk draw calls
		//this->rebuildVoxelChunkDrawCalls(renderChunk, voxelChunk, ceilingScale, chasmAnimPercent, updateStatics, true);
	}
}
