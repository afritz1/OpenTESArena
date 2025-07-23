#include "VoxelBoxCombineChunkManager.h"
#include "VoxelChunk.h"
#include "VoxelChunkManager.h"

void VoxelBoxCombineChunkManager::updateActiveChunks(Span<const ChunkInt2> newChunkPositions, Span<const ChunkInt2> freedChunkPositions,
	const VoxelChunkManager &voxelChunkManager)
{
	for (const ChunkInt2 chunkPos : freedChunkPositions)
	{
		const int chunkIndex = this->getChunkIndex(chunkPos);
		this->recycleChunk(chunkIndex);
	}

	for (const ChunkInt2 chunkPos : newChunkPositions)
	{
		const int spawnIndex = this->spawnChunk();
		VoxelBoxCombineChunk &boxCombineChunk = this->getChunkAtIndex(spawnIndex);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		boxCombineChunk.init(chunkPos, voxelChunk.height);
	}

	this->chunkPool.clear();
}

void VoxelBoxCombineChunkManager::update(Span<const ChunkInt2> activeChunkPositions, Span<const ChunkInt2> newChunkPositions,
	const VoxelChunkManager &voxelChunkManager)
{
	for (const ChunkInt2 chunkPos : newChunkPositions)
	{
		VoxelBoxCombineChunk &boxCombineChunk = this->getChunkAtPosition(chunkPos);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		Span<const VoxelInt3> dirtyVoxels = voxelChunk.dirtyShapeDefPositions;
		boxCombineChunk.update(dirtyVoxels, voxelChunk);
	}

	for (const ChunkInt2 chunkPos : activeChunkPositions)
	{
		VoxelBoxCombineChunk &boxCombineChunk = this->getChunkAtPosition(chunkPos);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);

		// Rebuild combined boxes due to change in mesh.
		Span<const VoxelInt3> dirtyShapeDefVoxels = voxelChunk.dirtyShapeDefPositions;
		Span<const VoxelInt3> dirtyFaceActivationVoxels = voxelChunk.dirtyFaceActivationPositions;
		boxCombineChunk.update(dirtyShapeDefVoxels, voxelChunk);
		boxCombineChunk.update(dirtyFaceActivationVoxels, voxelChunk);
	}
}
