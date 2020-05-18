#include "ChunkUtils.h"
#include "VoxelUtils.h"

#include "components/debug/Debug.h"

NewInt2 VoxelUtils::originalVoxelToNewVoxel(const OriginalInt2 &voxel, NSInt gridWidth, EWInt gridDepth)
{
	// These have a -1 since all .MIF start points are in the center of a voxel, giving a minimum
	// distance of 0.5 from grid sides, thus guaranteeing that no out-of-bounds grid access will
	// occur in those cases. This one doesn't have that bias (due to integers), and as such, values
	// may go out of grid range if using the unmodified dimensions.
	return NewInt2((gridWidth - 1) - voxel.y, (gridDepth - 1) - voxel.x);
}

OriginalInt2 VoxelUtils::newVoxelToOriginalVoxel(const NewInt2 &voxel, NSInt gridWidth, EWInt gridDepth)
{
	return VoxelUtils::originalVoxelToNewVoxel(voxel, gridWidth, gridDepth);
}

Double2 VoxelUtils::getTransformedVoxel(const Double2 &voxel, NSInt gridWidth, EWInt gridDepth)
{
	return Double2(
		static_cast<double>(gridWidth) - voxel.y,
		static_cast<double>(gridDepth) - voxel.x);
}

NewInt2 VoxelUtils::chunkVoxelToNewVoxel(const ChunkInt2 &chunk, const ChunkVoxelInt2 &voxel,
	NSInt gridWidth, EWInt gridDepth)
{
	const AbsoluteChunkVoxelInt2 absoluteChunkVoxel = voxel + (chunk * ChunkUtils::CHUNK_DIM);

	// Need to offset the east/west value since the chunk origin is to the left of the new voxel
	// grid origin, so there might be a gap.
	const EWInt nextHigherChunkX = ChunkUtils::getNextHigherChunkMultiple(gridDepth);
	const EWInt nextHigherXDiff = nextHigherChunkX - gridDepth;

	return NewInt2((gridWidth - 1) - absoluteChunkVoxel.y, absoluteChunkVoxel.x - nextHigherXDiff);
}

AbsoluteChunkVoxelInt2 VoxelUtils::chunkVoxelToAbsoluteChunkVoxel(const ChunkInt2 &chunk,
	const ChunkVoxelInt2 &voxel)
{
	return AbsoluteChunkVoxelInt2(
		(chunk.x * ChunkUtils::CHUNK_DIM) + voxel.x,
		(chunk.y * ChunkUtils::CHUNK_DIM) + voxel.y);
}

ChunkCoord VoxelUtils::newVoxelToChunkVoxel(const NewInt2 &voxel, NSInt gridWidth, EWInt gridDepth)
{
	// @todo: need to handle voxel outside grid.
	const EWInt nextHigherChunkX = ChunkUtils::getNextHigherChunkMultiple(gridDepth);
	const SNInt nextHigherChunkY = ChunkUtils::getNextHigherChunkMultiple(gridWidth);

	const AbsoluteChunkVoxelInt2 absoluteChunkVoxel(
		nextHigherChunkX - voxel.y + (nextHigherChunkX - gridDepth),
		(gridWidth - 1) - voxel.x);

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

ChunkInt2 VoxelUtils::newVoxelToChunk(const NewInt2 &voxel, NSInt gridWidth, EWInt gridDepth)
{
	const ChunkCoord chunkCoord = VoxelUtils::newVoxelToChunkVoxel(voxel, gridWidth, gridDepth);
	return chunkCoord.chunk;
}

AbsoluteChunkVoxelInt2 VoxelUtils::newVoxelToAbsoluteChunkVoxel(const NewInt2 &voxel,
	NSInt gridWidth, EWInt gridDepth)
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
