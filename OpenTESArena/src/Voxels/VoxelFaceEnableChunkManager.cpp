#include "VoxelFaceEnableChunkManager.h"
#include "VoxelChunk.h"
#include "VoxelChunkManager.h"

void VoxelFaceEnableChunkManager::updateActiveChunks(BufferView<const ChunkInt2> newChunkPositions, BufferView<const ChunkInt2> freedChunkPositions,
	const VoxelChunkManager &voxelChunkManager)
{
	for (const ChunkInt2 chunkPos : freedChunkPositions)
	{
		const int chunkIndex = this->getChunkIndex(chunkPos);
		this->recycleChunk(chunkIndex);
	}

	for (const ChunkInt2 chunkPos : newChunkPositions)
	{
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);

		const int spawnIndex = this->spawnChunk();
		VoxelFaceEnableChunk &faceEnableChunk = this->getChunkAtIndex(spawnIndex);
		faceEnableChunk.init(chunkPos, voxelChunk.getHeight());
	}

	this->chunkPool.clear();
}

void VoxelFaceEnableChunkManager::update(BufferView<const ChunkInt2> activeChunkPositions, BufferView<const ChunkInt2> newChunkPositions,
	const VoxelChunkManager &voxelChunkManager)
{
	for (const ChunkInt2 chunkPos : newChunkPositions)
	{
		VoxelFaceEnableChunk &faceEnableChunk = this->getChunkAtPosition(chunkPos);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		BufferView<const VoxelInt3> dirtyVoxels = voxelChunk.getDirtyShapeDefPositions();
		faceEnableChunk.update(dirtyVoxels, voxelChunk);
	}

	for (const ChunkInt2 chunkPos : activeChunkPositions)
	{
		VoxelFaceEnableChunk &faceEnableChunk = this->getChunkAtPosition(chunkPos);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		BufferView<const VoxelInt3> dirtyVoxels = voxelChunk.getDirtyShapeDefPositions();
		BufferView<const VoxelInt3> dirtyChasmWallInstVoxels = voxelChunk.getDirtyChasmWallInstPositions();
		faceEnableChunk.update(dirtyVoxels, voxelChunk);
		faceEnableChunk.update(dirtyChasmWallInstVoxels, voxelChunk);
	}
}
