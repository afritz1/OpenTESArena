#include "ChunkUtils.h"
#include "VoxelUtils.h"

#include "components/debug/Debug.h"

NewInt2 VoxelUtils::originalVoxelToNewVoxel(const OriginalInt2 &voxel)
{
	return NewInt2(voxel.y, voxel.x);
}

OriginalInt2 VoxelUtils::newVoxelToOriginalVoxel(const NewInt2 &voxel)
{
	return VoxelUtils::originalVoxelToNewVoxel(voxel);
}

Double2 VoxelUtils::getTransformedVoxel(const Double2 &voxel)
{
	return Double2(voxel.y, voxel.x);
}

NewInt2 VoxelUtils::chunkVoxelToNewVoxel(const ChunkInt2 &chunk, const ChunkVoxelInt2 &voxel)
{
	return (chunk * ChunkUtils::CHUNK_DIM) + voxel;
}

AbsoluteChunkVoxelInt2 VoxelUtils::chunkVoxelToAbsoluteChunkVoxel(const ChunkInt2 &chunk,
	const ChunkVoxelInt2 &voxel)
{
	return AbsoluteChunkVoxelInt2(
		(chunk.x * ChunkUtils::CHUNK_DIM) + voxel.x,
		(chunk.y * ChunkUtils::CHUNK_DIM) + voxel.y);
}

ChunkCoord VoxelUtils::newVoxelToChunkVoxel(const NewInt2 &voxel)
{
	// @todo: need to handle voxel outside grid.
	// @todo: probably want (int)Floor() instead of modulo.

	ChunkCoord chunkCoord;
	chunkCoord.chunk = ChunkInt2(voxel.x / ChunkUtils::CHUNK_DIM, voxel.y / ChunkUtils::CHUNK_DIM);
	chunkCoord.voxel = ChunkVoxelInt2(voxel.x % ChunkUtils::CHUNK_DIM, voxel.y % ChunkUtils::CHUNK_DIM);
	return chunkCoord;
}

ChunkInt2 VoxelUtils::newVoxelToChunk(const NewInt2 &voxel)
{
	const ChunkCoord chunkCoord = VoxelUtils::newVoxelToChunkVoxel(voxel);
	return chunkCoord.chunk;
}

AbsoluteChunkVoxelInt2 VoxelUtils::newVoxelToAbsoluteChunkVoxel(const NewInt2 &voxel)
{
	return voxel;
}

ChunkCoord VoxelUtils::absoluteChunkVoxelToChunkVoxel(const AbsoluteChunkVoxelInt2 &voxel)
{
	ChunkCoord chunkCoord;
	chunkCoord.chunk = ChunkInt2(voxel.x / ChunkUtils::CHUNK_DIM, voxel.y / ChunkUtils::CHUNK_DIM);
	chunkCoord.voxel = ChunkVoxelInt2(voxel.x % ChunkUtils::CHUNK_DIM, voxel.y % ChunkUtils::CHUNK_DIM);
	return chunkCoord;
}
