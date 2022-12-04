#include "EntityChunkManager.h"

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
