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
		return ChunkUtils::getNextHigherChunkMultiple(dim) / CHUNK_DIM;
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

bool ChunkUtils::isWithinActiveRange(const ChunkInt2 &chunk, const ChunkInt2 &other,
	int chunkDistance)
{
	DebugAssert(chunkDistance >= 1);
	const int xDiff = std::abs(other.x - chunk.x);
	const int yDiff = std::abs(other.y - chunk.y);
	return (xDiff <= chunkDistance) && (yDiff <= chunkDistance);
}
