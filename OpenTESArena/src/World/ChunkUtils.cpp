#include <cmath>

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

CoordDouble2 ChunkUtils::recalculateCoord(const ChunkInt2 &chunk, const VoxelDouble2 &point)
{
	// @todo: verify that negative coordinates are supported here.
	constexpr int chunkDim = ChunkUtils::CHUNK_DIM;
	const VoxelInt2 voxel = VoxelUtils::pointToVoxel(point);
	const SNInt chunkDiffX = ((voxel.x >= 0) ? voxel.x : (voxel.x - (chunkDim - 1))) / chunkDim;
	const WEInt chunkDiffZ = ((voxel.y >= 0) ? voxel.y : (voxel.y - (chunkDim - 1))) / chunkDim;
	const ChunkInt2 newChunk(chunk.x + chunkDiffX, chunk.y + chunkDiffZ);
	const VoxelDouble2 newPoint(
		std::fmod(point.x, static_cast<SNDouble>(chunkDim)),
		std::fmod(point.y, static_cast<WEDouble>(chunkDim)));
	return CoordDouble2(newChunk, newPoint);
}

CoordDouble3 ChunkUtils::recalculateCoord(const ChunkInt2 &chunk, const VoxelDouble3 &point)
{
	const CoordDouble2 coord = ChunkUtils::recalculateCoord(chunk, VoxelDouble2(point.x, point.z));
	const VoxelDouble2 &newPoint = coord.point;
	return CoordDouble3(coord.chunk, VoxelDouble3(newPoint.x, point.y, newPoint.y));
}

CoordInt2 ChunkUtils::recalculateCoord(const ChunkInt2 &chunk, const VoxelInt2 &voxel)
{
	// @todo: verify that negative coordinates are supported here.
	constexpr int chunkDim = ChunkUtils::CHUNK_DIM;
	const SNInt chunkDiffX = ((voxel.x >= 0) ? voxel.x : (voxel.x - (chunkDim - 1))) / chunkDim;
	const WEInt chunkDiffZ = ((voxel.y >= 0) ? voxel.y : (voxel.y - (chunkDim - 1))) / chunkDim;
	const ChunkInt2 newChunk(chunk.x + chunkDiffX, chunk.y + chunkDiffZ);
	const VoxelInt2 newVoxel(voxel.x % ChunkUtils::CHUNK_DIM, voxel.y % ChunkUtils::CHUNK_DIM);
	return CoordInt2(newChunk, newVoxel);
}

CoordInt3 ChunkUtils::recalculateCoord(const ChunkInt2 &chunk, const VoxelInt3 &voxel)
{
	const CoordInt2 coord = ChunkUtils::recalculateCoord(chunk, VoxelInt2(voxel.x, voxel.z));
	const VoxelInt2 &newVoxel = coord.voxel;
	return CoordInt3(coord.chunk, VoxelInt3(newVoxel.x, voxel.y, newVoxel.y));
}
