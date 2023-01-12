#include "EntityChunkManager.h"
#include "../Voxels/VoxelChunk.h"
#include "../Voxels/VoxelChunkManager.h"

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

void EntityChunkManager::populateChunk(EntityChunk &entityChunk, const VoxelChunk &voxelChunk, double ceilingScale,
	TextureManager &textureManager, Renderer &renderer)
{
	// @todo
}

void EntityChunkManager::update(double dt, const BufferView<const ChunkInt2> &activeChunkPositions,
	const BufferView<const ChunkInt2> &newChunkPositions, const BufferView<const ChunkInt2> &freedChunkPositions,
	const CoordDouble3 &playerCoord, double ceilingScale, const VoxelChunkManager &voxelChunkManager,
	TextureManager &textureManager, Renderer &renderer)
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

		this->populateChunk(entityChunk, voxelChunk, ceilingScale, textureManager, renderer);
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
