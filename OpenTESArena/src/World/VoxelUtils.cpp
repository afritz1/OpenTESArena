#include "VoxelUtils.h"

#include "components/debug/Debug.h"

int VoxelUtils::getNextHigherChunkMultiple(int coord)
{
	const int remainder = coord % VoxelUtils::CHUNK_DIM;
	return (remainder == 0) ? coord : (coord + VoxelUtils::CHUNK_DIM - remainder);
}

void VoxelUtils::getChunkCounts(NSInt gridWidth, EWInt gridDepth, EWInt *outChunkCountX,
	SNInt *outChunkCountY)
{
	auto chunksForDimension = [](int dim)
	{
		return VoxelUtils::getNextHigherChunkMultiple(dim) / CHUNK_DIM;
	};

	*outChunkCountX = chunksForDimension(gridDepth);
	*outChunkCountY = chunksForDimension(gridWidth);
}

void VoxelUtils::getPotentiallyVisibleChunkCounts(int chunkDistance, EWInt *outChunkCountX,
	SNInt *outChunkCountY)
{
	DebugAssert(chunkDistance >= 1);
	const int count = 1 + (chunkDistance * 2);
	*outChunkCountX = count;
	*outChunkCountY = count;
}

void VoxelUtils::getSurroundingChunks(const ChunkInt2 &chunk, int chunkDist, ChunkInt2 *outMinChunk,
	ChunkInt2 *outMaxChunk)
{
	DebugAssert(chunkDist >= 1);
	*outMinChunk = ChunkInt2(chunk.x - chunkDist, chunk.y - chunkDist);
	*outMaxChunk = ChunkInt2(chunk.x + chunkDist, chunk.y + chunkDist);
}

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
	const AbsoluteChunkVoxelInt2 absoluteChunkVoxel = voxel + (chunk * CHUNK_DIM);

	// Need to offset the east/west value since the chunk origin is to the left of the new voxel
	// grid origin, so there might be a gap.
	const EWInt nextHigherChunkX = VoxelUtils::getNextHigherChunkMultiple(gridDepth);
	const EWInt nextHigherXDiff = nextHigherChunkX - gridDepth;

	return NewInt2((gridWidth - 1) - absoluteChunkVoxel.y, absoluteChunkVoxel.x - nextHigherXDiff);
}

AbsoluteChunkVoxelInt2 VoxelUtils::chunkVoxelToAbsoluteChunkVoxel(const ChunkInt2 &chunk,
	const ChunkVoxelInt2 &voxel)
{
	return AbsoluteChunkVoxelInt2(
		(chunk.x * VoxelUtils::CHUNK_DIM) + voxel.x,
		(chunk.y * VoxelUtils::CHUNK_DIM) + voxel.y);
}

ChunkCoord VoxelUtils::newVoxelToChunkVoxel(const NewInt2 &voxel, NSInt gridWidth, EWInt gridDepth)
{
	// @todo: need to handle voxel outside grid.
	const EWInt nextHigherChunkX = VoxelUtils::getNextHigherChunkMultiple(gridDepth);
	const SNInt nextHigherChunkY = VoxelUtils::getNextHigherChunkMultiple(gridWidth);

	const AbsoluteChunkVoxelInt2 absoluteChunkVoxel(
		nextHigherChunkX - voxel.y + (nextHigherChunkX - gridDepth),
		(gridWidth - 1) - voxel.x);

	ChunkCoord chunkCoord;
	chunkCoord.chunk = ChunkInt2(absoluteChunkVoxel.x / CHUNK_DIM, absoluteChunkVoxel.y / CHUNK_DIM);
	// @todo: probably want (int)Floor() instead of modulo.
	chunkCoord.voxel = ChunkVoxelInt2(absoluteChunkVoxel.x % CHUNK_DIM, absoluteChunkVoxel.y % CHUNK_DIM);
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
	chunkCoord.chunk = ChunkInt2(voxel.x / VoxelUtils::CHUNK_DIM, voxel.y / VoxelUtils::CHUNK_DIM);
	chunkCoord.voxel = ChunkVoxelInt2(voxel.x % VoxelUtils::CHUNK_DIM, voxel.y % VoxelUtils::CHUNK_DIM);
	return chunkCoord;
}
