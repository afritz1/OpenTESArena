#include "VoxelChunkManager.h"
#include "VoxelVisibilityChunkManager.h"

void VoxelVisibilityChunkManager::update(BufferView<const ChunkInt2> newChunkPositions, BufferView<const ChunkInt2> freedChunkPositions,
	const RenderCamera &camera, double ceilingScale, const VoxelChunkManager &voxelChunkManager)
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
		VoxelVisibilityChunk &visChunk = this->getChunkAtIndex(spawnIndex);
		visChunk.init(chunkPos, voxelChunk.getHeight(), ceilingScale);
	}

	this->chunkPool.clear();

	for (ChunkPtr &chunkPtr : this->activeChunks)
	{
		chunkPtr->update(camera);
	}
}
