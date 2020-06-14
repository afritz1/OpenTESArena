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

NewInt2 VoxelUtils::chunkVoxelToNewVoxel(const ChunkInt2 &chunk, const ChunkVoxelInt2 &voxel,
	SNInt gridWidth, WEInt gridDepth)
{
	const AbsoluteChunkVoxelInt2 absoluteChunkVoxel = (chunk * ChunkUtils::CHUNK_DIM) + voxel;
	const WEInt nextHigherChunkX = ChunkUtils::getNextHigherChunkMultiple(gridDepth);
	return NewInt2(absoluteChunkVoxel.y, (nextHigherChunkX - 1) - absoluteChunkVoxel.x);
}

AbsoluteChunkVoxelInt2 VoxelUtils::chunkVoxelToAbsoluteChunkVoxel(const ChunkInt2 &chunk,
	const ChunkVoxelInt2 &voxel)
{
	return AbsoluteChunkVoxelInt2(
		(chunk.x * ChunkUtils::CHUNK_DIM) + voxel.x,
		(chunk.y * ChunkUtils::CHUNK_DIM) + voxel.y);
}

ChunkCoord VoxelUtils::newVoxelToChunkVoxel(const NewInt2 &voxel, SNInt gridWidth, WEInt gridDepth)
{
	// @todo: need to handle voxel outside grid.
	const WEInt nextHigherChunkX = ChunkUtils::getNextHigherChunkMultiple(gridDepth);
	const SNInt nextHigherChunkY = ChunkUtils::getNextHigherChunkMultiple(gridWidth);
	const AbsoluteChunkVoxelInt2 absoluteChunkVoxel((nextHigherChunkX - 1) - voxel.y, voxel.x);

	ChunkCoord chunkCoord;
	chunkCoord.chunk = ChunkInt2(
		absoluteChunkVoxel.x / ChunkUtils::CHUNK_DIM,
		absoluteChunkVoxel.y / ChunkUtils::CHUNK_DIM);
	// @todo: probably want (int)Floor() instead of modulo.
	chunkCoord.voxel = ChunkVoxelInt2(
		absoluteChunkVoxel.x % ChunkUtils::CHUNK_DIM,
		absoluteChunkVoxel.y % ChunkUtils::CHUNK_DIM);

	return chunkCoord;
}

ChunkInt2 VoxelUtils::newVoxelToChunk(const NewInt2 &voxel, SNInt gridWidth, WEInt gridDepth)
{
	const ChunkCoord chunkCoord = VoxelUtils::newVoxelToChunkVoxel(voxel, gridWidth, gridDepth);
	return chunkCoord.chunk;
}

AbsoluteChunkVoxelInt2 VoxelUtils::newVoxelToAbsoluteChunkVoxel(const NewInt2 &voxel,
	SNInt gridWidth, WEInt gridDepth)
{
	const ChunkCoord chunkCoord = VoxelUtils::newVoxelToChunkVoxel(voxel, gridWidth, gridDepth);
	return VoxelUtils::chunkVoxelToAbsoluteChunkVoxel(chunkCoord.chunk, chunkCoord.voxel);
}

ChunkCoord VoxelUtils::absoluteChunkVoxelToChunkVoxel(const AbsoluteChunkVoxelInt2 &voxel)
{
	ChunkCoord chunkCoord;
	chunkCoord.chunk = ChunkInt2(voxel.x / ChunkUtils::CHUNK_DIM, voxel.y / ChunkUtils::CHUNK_DIM);
	chunkCoord.voxel = ChunkVoxelInt2(voxel.x % ChunkUtils::CHUNK_DIM, voxel.y % ChunkUtils::CHUNK_DIM);
	return chunkCoord;
}
