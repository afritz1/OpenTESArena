#include "ChunkUtils.h"

#include "components/debug/Debug.h"

int ChunkUtils::getNextHigherChunkMultiple(int coord)
{
	const int remainder = coord % ChunkUtils::CHUNK_DIM;
	return (remainder == 0) ? coord : (coord + ChunkUtils::CHUNK_DIM - remainder);
}

void ChunkUtils::getChunkCounts(SNInt gridWidth, WEInt gridDepth, SNInt *outChunkCountX,
	WEInt *outChunkCountZ)
{
	auto chunksForDimension = [](int dim)
	{
		return ChunkUtils::getNextHigherChunkMultiple(dim) / ChunkUtils::CHUNK_DIM;
	};

	*outChunkCountX = chunksForDimension(gridWidth);
	*outChunkCountZ = chunksForDimension(gridDepth);
}

void ChunkUtils::getPotentiallyVisibleChunkCounts(int chunkDistance, SNInt *outChunkCountX,
	WEInt *outChunkCountZ)
{
	DebugAssert(chunkDistance >= 1);
	const int count = 1 + (chunkDistance * 2);
	*outChunkCountX = count;
	*outChunkCountZ = count;
}

void ChunkUtils::getSurroundingChunks(const ChunkInt2 &chunk, int chunkDistance,
	ChunkInt2 *outMinChunk, ChunkInt2 *outMaxChunk)
{
	DebugAssert(chunkDistance >= 1);
	*outMinChunk = ChunkInt2(chunk.x - chunkDistance, chunk.y - chunkDistance);
	*outMaxChunk = ChunkInt2(chunk.x + chunkDistance, chunk.y + chunkDistance);
}

bool ChunkUtils::touchesLevelDimensions(const ChunkInt2 &chunk, SNInt levelWidth, WEInt levelDepth)
{
	if ((chunk.x < 0) || (chunk.y < 0))
	{
		return false;
	}

	SNInt chunkCountX;
	WEInt chunkCountZ;
	ChunkUtils::getChunkCounts(levelWidth, levelDepth, &chunkCountX, &chunkCountZ);
	return ((chunkCountX - 1) >= chunk.x) && ((chunkCountZ - 1) >= chunk.y);
}

bool ChunkUtils::isWithinActiveRange(const ChunkInt2 &chunk, const ChunkInt2 &other,
	int chunkDistance)
{
	DebugAssert(chunkDistance >= 1);
	const int xDiff = std::abs(other.x - chunk.x);
	const int yDiff = std::abs(other.y - chunk.y);
	return (xDiff <= chunkDistance) && (yDiff <= chunkDistance);
}

ChunkCoord2D ChunkUtils::recalculateCoord(const ChunkInt2 &chunk, const VoxelInt2 &voxel)
{
	// @todo: verify that negative coordinates are supported here.
	const SNInt chunkDiffX = ChunkUtils::getNextHigherChunkMultiple(voxel.x) / ChunkUtils::CHUNK_DIM;
	const WEInt chunkDiffZ = ChunkUtils::getNextHigherChunkMultiple(voxel.y) / ChunkUtils::CHUNK_DIM;
	const ChunkInt2 newChunk(chunk.x + chunkDiffX, chunk.y + chunkDiffZ);
	const VoxelInt2 newVoxel(voxel.x % ChunkUtils::CHUNK_DIM, voxel.y % ChunkUtils::CHUNK_DIM);
	return ChunkCoord2D(newChunk, newVoxel);
}

ChunkCoord3D ChunkUtils::recalculateCoord(const ChunkInt2 &chunk, const VoxelInt3 &voxel)
{
	const ChunkCoord2D chunkCoord = ChunkUtils::recalculateCoord(chunk, VoxelInt2(voxel.x, voxel.z));
	const ChunkInt2 &newChunk = chunkCoord.chunk;
	const VoxelInt2 &newVoxel = chunkCoord.voxel;
	return ChunkCoord3D(newChunk, VoxelInt3(newVoxel.x, voxel.y, newVoxel.y));
}
