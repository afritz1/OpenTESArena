#include <algorithm>

#include "CombatLogic.h"
#include "../Entities/EntityChunkManager.h"
#include "../Math/BoundingBox.h"
#include "../Voxels/VoxelChunkManager.h"

CombatHitSearchResult::CombatHitSearchResult()
{
	this->voxelCount = 0;
	std::fill(std::begin(this->entities), std::end(this->entities), -1);
	this->entityCount = 0;
}

BufferView<const WorldInt3> CombatHitSearchResult::getVoxels() const
{
	return BufferView<const WorldInt3>(this->voxels, this->voxelCount);
}

BufferView<const EntityInstanceID> CombatHitSearchResult::getEntities() const
{
	return BufferView<const EntityInstanceID>(this->entities, this->entityCount);
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
				const VoxelChunk *voxelChunk = voxelChunkManager.tryGetChunkAtPosition(searchVoxelCoord.chunk);
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
			const EntityChunk *entityChunk = entityChunkManager.tryGetChunkAtPosition(searchChunkPos);
			if (entityChunk != nullptr)
			{
				for (const EntityInstanceID entityInstID : entityChunk->entityIDs)
				{
					const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
					const WorldDouble3 entityPosition = entityChunkManager.getEntityPosition(entityInst.positionID);
					const BoundingBox3D &entityBBox = entityChunkManager.getEntityBoundingBox(entityInst.bboxID);

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
