#include <algorithm>

#include "RenderLightChunk.h"

void RenderLightChunk::init(const ChunkInt2 &position, int height)
{
	Chunk::init(position, height);
	this->lightIdLists.init(ChunkUtils::CHUNK_DIM, height, ChunkUtils::CHUNK_DIM);
}

void RenderLightChunk::setVoxelDirty(const VoxelInt3 &position)
{
	if (!this->isValidVoxel(position.x, position.y, position.z))
	{
		DebugLogWarning("Invalid dirty light position (" + position.toString() + ") in chunk (" + this->getPosition().toString() + ").");
		return;
	}

	const auto iter = std::find(this->dirtyVoxels.begin(), this->dirtyVoxels.end(), position);
	if (iter == this->dirtyVoxels.end())
	{
		this->dirtyVoxels.emplace_back(position);
	}
}

void RenderLightChunk::clear()
{
	Chunk::clear();
	this->lightIdLists.clear();
	this->dirtyVoxels.clear();
}
