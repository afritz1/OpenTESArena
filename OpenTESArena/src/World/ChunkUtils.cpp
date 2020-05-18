#include "ChunkUtils.h"

#include "components/debug/Debug.h"

int ChunkUtils::getNextHigherChunkMultiple(int coord)
{
	const int remainder = coord % ChunkUtils::CHUNK_DIM;
	return (remainder == 0) ? coord : (coord + ChunkUtils::CHUNK_DIM - remainder);
}

void ChunkUtils::getChunkCounts(NSInt gridWidth, EWInt gridDepth, EWInt *outChunkCountX,
	SNInt *outChunkCountY)
{
	auto chunksForDimension = [](int dim)
	{
		return ChunkUtils::getNextHigherChunkMultiple(dim) / CHUNK_DIM;
	};

	*outChunkCountX = chunksForDimension(gridDepth);
	*outChunkCountY = chunksForDimension(gridWidth);
}

void ChunkUtils::getPotentiallyVisibleChunkCounts(int chunkDistance, EWInt *outChunkCountX,
	SNInt *outChunkCountY)
{
	DebugAssert(chunkDistance >= 1);
	const int count = 1 + (chunkDistance * 2);
	*outChunkCountX = count;
	*outChunkCountY = count;
}

void ChunkUtils::getSurroundingChunks(const ChunkInt2 &chunk, int chunkDist, ChunkInt2 *outMinChunk,
	ChunkInt2 *outMaxChunk)
{
	DebugAssert(chunkDist >= 1);
	*outMinChunk = ChunkInt2(chunk.x - chunkDist, chunk.y - chunkDist);
	*outMaxChunk = ChunkInt2(chunk.x + chunkDist, chunk.y + chunkDist);
}