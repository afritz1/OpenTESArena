#include "VoxelChunk.h"
#include "VoxelChunkManager.h"
#include "VoxelFaceCombineChunkManager.h"
#include "VoxelFaceEnableChunkManager.h"

void VoxelFaceCombineChunkManager::updateActiveChunks(Span<const ChunkInt2> newChunkPositions, Span<const ChunkInt2> freedChunkPositions,
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

void VoxelFaceCombineChunkManager::update(Span<const ChunkInt2> activeChunkPositions, Span<const ChunkInt2> newChunkPositions,
	const VoxelChunkManager &voxelChunkManager, const VoxelFaceEnableChunkManager &voxelFaceEnableChunkManager)
{
	for (const ChunkInt2 chunkPos : newChunkPositions)
	{
		VoxelFaceCombineChunk &faceCombineChunk = this->getChunkAtPosition(chunkPos);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		Span<const VoxelInt3> dirtyVoxels = voxelChunk.getDirtyShapeDefPositions();
		const VoxelFaceEnableChunk &faceEnableChunk = voxelFaceEnableChunkManager.getChunkAtPosition(chunkPos);
		faceCombineChunk.update(dirtyVoxels, voxelChunk, faceEnableChunk);
	}

	for (const ChunkInt2 chunkPos : activeChunkPositions)
	{
		VoxelFaceCombineChunk &faceCombineChunk = this->getChunkAtPosition(chunkPos);
		const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		const VoxelFaceEnableChunk &faceEnableChunk = voxelFaceEnableChunkManager.getChunkAtPosition(chunkPos);

		// Rebuild combined faces due to change in mesh.
		Span<const VoxelInt3> dirtyShapeDefVoxels = voxelChunk.getDirtyShapeDefPositions();
		faceCombineChunk.update(dirtyShapeDefVoxels, voxelChunk, faceEnableChunk);

		Span<const VoxelInt3> dirtyFaceActivationVoxels = voxelChunk.getDirtyFaceActivationPositions();
		faceCombineChunk.update(dirtyFaceActivationVoxels, voxelChunk, faceEnableChunk);

		// Rebuild combined faces due to changes in material.
		Span<const VoxelInt3> dirtyFadeAnimInstVoxels = voxelChunk.getDirtyFadeAnimInstPositions();
		faceCombineChunk.update(dirtyFadeAnimInstVoxels, voxelChunk, faceEnableChunk);
	}
}
