#include "EntityChunkManager.h"
#include "EntityDefinitionLibrary.h"
#include "EntityType.h"
#include "../Game/CardinalDirection.h"
#include "../Math/Random.h"
#include "../Voxels/VoxelChunk.h"
#include "../Voxels/VoxelChunkManager.h"
#include "../World/LevelDefinition.h"
#include "../World/LevelInfoDefinition.h"
#include "../World/MapDefinition.h"
#include "../World/MapType.h"

/*void VoxelChunkManager::populateChunkEntities(VoxelChunk &chunk, const LevelDefinition &levelDefinition,
	const LevelInfoDefinition &levelInfoDefinition, const LevelInt2 &levelOffset,
	const EntityGeneration::EntityGenInfo &entityGenInfo,
	const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
	const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	TextureManager &textureManager, EntityManager &entityManager)
{
	SNInt startX, endX;
	int startY, endY;
	WEInt startZ, endZ;
	ChunkUtils::GetWritingRanges(levelOffset, levelDefinition.getWidth(), levelDefinition.getHeight(),
		levelDefinition.getDepth(), &startX, &startY, &startZ, &endX, &endY, &endZ);

	// Cosmetic random (initial creature sound timing, etc.).
	Random random;

	for (int i = 0; i < levelDefinition.getEntityPlacementDefCount(); i++)
	{
		const LevelDefinition::EntityPlacementDef &placementDef = levelDefinition.getEntityPlacementDef(i);
		const LevelDefinition::EntityDefID levelEntityDefID = placementDef.id;
		const EntityDefinition &entityDef = levelInfoDefinition.getEntityDef(levelEntityDefID);
		const EntityDefinition::Type entityDefType = entityDef.getType();
		const EntityType entityType = EntityUtils::getEntityTypeFromDefType(entityDefType);
		const EntityAnimationDefinition &animDef = entityDef.getAnimDef();

		std::optional<EntityDefID> entityDefID;
		for (const LevelDouble3 &position : placementDef.positions)
		{
			const double ceilingScale = levelInfoDefinition.getCeilingScale();
			const LevelInt3 voxelPosition = VoxelUtils::pointToVoxel(position, ceilingScale);
			if (ChunkUtils::IsInWritingRange(voxelPosition, startX, endX, startY, endY, startZ, endZ))
			{
				if (!entityDefID.has_value())
				{
					entityDefID = entityManager.addEntityDef(EntityDefinition(entityDef), entityDefLibrary);
				}

				const VoxelDouble3 point = ChunkUtils::MakeChunkPointFromLevel(position, startX, startY, startZ);
				Entity *entity = EntityGeneration::makeEntity(entityType, entityDefType, *entityDefID,
					entityDef, animDef, entityGenInfo, random, entityManager);

				// Set entity position in chunk last. This has the potential to change the entity's chunk
				// and invalidate the local entity pointer.
				const CoordDouble2 coord(chunk.getPosition(), VoxelDouble2(point.x, point.z));
				entity->setPosition(coord, entityManager);
			}
		}
	}

	if (citizenGenInfo.has_value())
	{
		// Spawn citizens if the total active limit has not been reached.
		const int currentCitizenCount = CitizenUtils::getCitizenCount(entityManager);
		const int remainingCitizensToSpawn = std::min(
			CitizenUtils::MAX_ACTIVE_CITIZENS - currentCitizenCount, CitizenUtils::CITIZENS_PER_CHUNK);

		for (int i = 0; i < remainingCitizensToSpawn; i++)
		{
			if (!CitizenUtils::trySpawnCitizenInChunk(chunk, *citizenGenInfo, random, binaryAssetLibrary,
				textureManager, entityManager))
			{
				DebugLogWarning("Couldn't spawn citizen in chunk \"" + chunk.getPosition().toString() + "\".");
			}
		}
	}
}*/

EntityDefID EntityChunkManager::addEntityDef(EntityDefinition &&def, const EntityDefinitionLibrary &defLibrary)
{
	const int libraryDefCount = defLibrary.getDefinitionCount();
	const EntityDefID defID = static_cast<EntityDefID>(libraryDefCount + this->entityDefs.size());
	this->entityDefs.emplace(defID, std::move(def));
	return defID;
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
	const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	TextureManager &textureManager)
{
	SNInt startX, endX;
	int startY, endY;
	WEInt startZ, endZ;
	ChunkUtils::GetWritingRanges(levelOffset, levelDefinition.getWidth(), levelDefinition.getHeight(),
		levelDefinition.getDepth(), &startX, &startY, &startZ, &endX, &endY, &endZ);

	// Cosmetic random (initial creature sound timing, etc.).
	Random random;

	for (int i = 0; i < levelDefinition.getEntityPlacementDefCount(); i++)
	{
		const LevelDefinition::EntityPlacementDef &placementDef = levelDefinition.getEntityPlacementDef(i);
		const LevelDefinition::EntityDefID levelEntityDefID = placementDef.id;
		const EntityDefinition &entityDef = levelInfoDefinition.getEntityDef(levelEntityDefID);
		const EntityDefinition::Type entityDefType = entityDef.getType();
		const EntityType entityType = EntityUtils::getEntityTypeFromDefType(entityDefType);
		const EntityAnimationDefinition &animDef = entityDef.getAnimDef();

		std::optional<EntityDefID> entityDefID;
		for (const LevelDouble3 &position : placementDef.positions)
		{
			const double ceilingScale = levelInfoDefinition.getCeilingScale();
			const LevelInt3 voxelPosition = VoxelUtils::pointToVoxel(position, ceilingScale);
			if (ChunkUtils::IsInWritingRange(voxelPosition, startX, endX, startY, endY, startZ, endZ))
			{
				if (!entityDefID.has_value())
				{
					entityDefID = this->addEntityDef(EntityDefinition(entityDef), entityDefLibrary);
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
						// @todo: init creature sound seconds w/ Random (need another pool for creature sound seconds probably)
					}
				}
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
	double ceilingScale, const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	TextureManager &textureManager, Renderer &renderer)
{
	const ChunkInt2 &chunkPos = entityChunk.getPosition();

	// Populate all or part of the chunk from a level definition depending on the world type.
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
				citizenGenInfo, entityDefLibrary, binaryAssetLibrary, textureManager);
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
				citizenGenInfo, entityDefLibrary, binaryAssetLibrary, textureManager);
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
			citizenGenInfo, entityDefLibrary, binaryAssetLibrary, textureManager);
	}
}

void EntityChunkManager::update(double dt, const BufferView<const ChunkInt2> &activeChunkPositions,
	const BufferView<const ChunkInt2> &newChunkPositions, const BufferView<const ChunkInt2> &freedChunkPositions,
	const CoordDouble3 &playerCoord, const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
	const EntityGeneration::EntityGenInfo &entityGenInfo, const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
	double ceilingScale, const VoxelChunkManager &voxelChunkManager, const EntityDefinitionLibrary &entityDefLibrary,
	const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager, Renderer &renderer)
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
			ceilingScale, entityDefLibrary, binaryAssetLibrary, textureManager, renderer);
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

		// @todo: citizen spawning and management by player distance

		// @todo: rebuild entity chunk draw calls
		//this->rebuildVoxelChunkDrawCalls(renderChunk, voxelChunk, ceilingScale, chasmAnimPercent, updateStatics, true);
	}
}
