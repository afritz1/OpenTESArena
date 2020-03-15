#include "VoxelUtils.h"

NewInt2 VoxelUtils::chunkVoxelToNewVoxel(const ChunkInt2 &chunk, const ChunkVoxelInt2 &voxel,
	NSInt gridWidth, EWInt gridDepth)
{
	const Int2 absoluteChunkVoxel = voxel + (chunk * CHUNK_DIM);
	return NewInt2((gridWidth - 1) - absoluteChunkVoxel.y, absoluteChunkVoxel.x);
}
