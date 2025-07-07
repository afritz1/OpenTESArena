#include "VoxelChunk.h"
#include "VoxelChunkManager.h"
#include "VoxelFaceEnableChunkManager.h"

void VoxelFaceEnableChunkManager::updateActiveChunks(Span<const ChunkInt2> newChunkPositions, Span<const ChunkInt2> freedChunkPositions,
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
		faceEnableChunk.init(chunkPos, voxelChunk.height);
	}

	this->chunkPool.clear();
}

void VoxelFaceEnableChunkManager::update(Span<const ChunkInt2> activeChunkPositions, Span<const ChunkInt2> newChunkPositions,
	const VoxelChunkManager &voxelChunkManager)
{
	for (const ChunkInt2 chunkPos : newChunkPositions)
	{
		VoxelFaceEnableChunk &faceEnableChunk = this->getChunkAtPosition(chunkPos);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		Span<const VoxelInt3> dirtyVoxels = voxelChunk.dirtyShapeDefPositions;
		faceEnableChunk.update(dirtyVoxels, voxelChunk);
	}

	for (const ChunkInt2 chunkPos : activeChunkPositions)
	{
		VoxelFaceEnableChunk &faceEnableChunk = this->getChunkAtPosition(chunkPos);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		Span<const VoxelInt3> dirtyShapeDefVoxels = voxelChunk.dirtyShapeDefPositions;
		Span<const VoxelInt3> dirtyFaceActivationVoxels = voxelChunk.dirtyFaceActivationPositions;
		faceEnableChunk.update(dirtyShapeDefVoxels, voxelChunk);
		faceEnableChunk.update(dirtyFaceActivationVoxels, voxelChunk);
	}
}
