#include <algorithm>

#include "RenderLightChunk.h"

void RenderLightChunk::init(const ChunkInt2 &position, int height)
{
	Chunk::init(position, height);
	this->lightIdLists.init(ChunkUtils::CHUNK_DIM, height, ChunkUtils::CHUNK_DIM);
	this->dirtyVoxels.init(ChunkUtils::CHUNK_DIM, height, ChunkUtils::CHUNK_DIM);
	this->dirtyVoxels.fill(false);
}

void RenderLightChunk::setVoxelDirty(const VoxelInt3 &position)
{
	if (!this->isValidVoxel(position.x, position.y, position.z))
	{
		DebugLogWarning("Invalid dirty light position (" + position.toString() + ") in chunk (" + this->position.toString() + ").");
		return;
	}

	const bool isAlreadyDirty = this->dirtyVoxels.get(position.x, position.y, position.z);
	if (!isAlreadyDirty)
	{
		this->dirtyVoxelPositions.emplace_back(position);
		this->dirtyVoxels.set(position.x, position.y, position.z, true);
	}
}

void RenderLightChunk::clearDirtyVoxels()
{
	this->dirtyVoxelPositions.clear();
	this->dirtyVoxels.fill(false);
}

void RenderLightChunk::clear()
{
	Chunk::clear();
	this->lightIdLists.clear();
	this->dirtyVoxelPositions.clear();
	this->dirtyVoxels.clear();
}
