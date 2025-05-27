#include "EntityChunkManager.h"
#include "EntityVisibilityChunkManager.h"
#include "../Voxels/VoxelChunkManager.h"

void EntityVisibilityChunkManager::update(BufferView<const ChunkInt2> activeChunkPositions, BufferView<const ChunkInt2> newChunkPositions,
	BufferView<const ChunkInt2> freedChunkPositions, const RenderCamera &camera, double ceilingScale,
	const VoxelChunkManager &voxelChunkManager, const EntityChunkManager &entityChunkManager)
{
	for (const ChunkInt2 &chunkPos : freedChunkPositions)
	{
		const int chunkIndex = this->getChunkIndex(chunkPos);
		this->recycleChunk(chunkIndex);
	}

	for (const ChunkInt2 &chunkPos : newChunkPositions)
	{
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);

		const int spawnIndex = this->spawnChunk();
		EntityVisibilityChunk &visChunk = this->getChunkAtIndex(spawnIndex);
		visChunk.init(chunkPos, voxelChunk.getHeight());
	}

	this->chunkPool.clear();

	for (const ChunkInt2 &chunkPos : activeChunkPositions)
	{
		EntityVisibilityChunk &visChunk = this->getChunkAtPosition(chunkPos);
		const EntityChunk &entityChunk = entityChunkManager.getChunkAtPosition(chunkPos);
		visChunk.update(camera, ceilingScale, entityChunk, entityChunkManager);
	}
}
