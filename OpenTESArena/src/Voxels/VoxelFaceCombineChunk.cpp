#include <algorithm>

#include "VoxelFaceCombineChunk.h"

VoxelFacesEntry::VoxelFacesEntry()
{
	this->clear();
}

void VoxelFacesEntry::clear()
{
	std::fill(std::begin(this->faces), std::end(this->faces), -1);
}

void VoxelFaceCombineChunk::init(const ChunkInt2 &position, int height)
{
	Chunk::init(position, height);
}

void VoxelFaceCombineChunk::update(const BufferView<const VoxelInt3> dirtyVoxels)
{
	// @todo search through all faces of all dirty voxels, try to make combined face entries
}

void VoxelFaceCombineChunk::clear()
{
	Chunk::clear();
	this->entries.clear();
}
