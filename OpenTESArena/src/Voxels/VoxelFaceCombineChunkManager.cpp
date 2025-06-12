#include "VoxelChunk.h"
#include "VoxelChunkManager.h"
#include "VoxelFaceCombineChunkManager.h"
#include "VoxelFaceEnableChunkManager.h"

void VoxelFaceCombineChunkManager::updateActiveChunks(BufferView<const ChunkInt2> newChunkPositions, BufferView<const ChunkInt2> freedChunkPositions,
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
		VoxelFaceCombineChunk &faceCombineChunk = this->getChunkAtIndex(spawnIndex);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		faceCombineChunk.init(chunkPos, voxelChunk.height);
	}

	this->chunkPool.clear();
}

void VoxelFaceCombineChunkManager::update(BufferView<const ChunkInt2> activeChunkPositions, BufferView<const ChunkInt2> newChunkPositions,
	const VoxelChunkManager &voxelChunkManager, const VoxelFaceEnableChunkManager &voxelFaceEnableChunkManager)
{
	for (const ChunkInt2 chunkPos : newChunkPositions)
	{
		VoxelFaceCombineChunk &faceCombineChunk = this->getChunkAtPosition(chunkPos);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		BufferView<const VoxelInt3> dirtyVoxels = voxelChunk.getDirtyShapeDefPositions();
		const VoxelFaceEnableChunk &faceEnableChunk = voxelFaceEnableChunkManager.getChunkAtPosition(chunkPos);
		faceCombineChunk.update(dirtyVoxels, voxelChunk, faceEnableChunk);
	}

	for (const ChunkInt2 chunkPos : activeChunkPositions)
	{
		VoxelFaceCombineChunk &faceCombineChunk = this->getChunkAtPosition(chunkPos);

		// Rebuild combined faces due to changes in material.
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		BufferView<const VoxelInt3> dirtyFadeAnimInstVoxels = voxelChunk.getDirtyFadeAnimInstPositions();
		const VoxelFaceEnableChunk &faceEnableChunk = voxelFaceEnableChunkManager.getChunkAtPosition(chunkPos);
		faceCombineChunk.update(dirtyFadeAnimInstVoxels, voxelChunk, faceEnableChunk);
	}
}
