#ifndef CHUNK_UTILS_H
#define CHUNK_UTILS_H

#include "VoxelUtils.h"

namespace ChunkUtils
{
	// Number of voxels per side on a chunk.
	constexpr int CHUNK_DIM = 64;

	// I.e., given 23, returns 64.
	int getNextHigherChunkMultiple(int coord);

	// Gets the number of chunks in each dimension required to fit the given area that's in
	// new voxel grid space.
	void getChunkCounts(NSInt gridWidth, EWInt gridDepth, EWInt *outChunkCountX, SNInt *outChunkCountY);

	// Gets the number of chunks that are potentially visible at any given time.
	void getPotentiallyVisibleChunkCounts(int chunkDistance, EWInt *outChunkCountX, SNInt *outChunkCountY);

	// Gets chunks in an NxN pattern around the given chunk. Useful for potentially visible chunk
	// coordinates around the camera position. Chunk distance is the distance away from the given
	// chunk in X or Y to reach (to obtain 3x3, 5x5, etc.).
	void getSurroundingChunks(const ChunkInt2 &chunk, int chunkDist, ChunkInt2 *outMinChunk,
		ChunkInt2 *outMaxChunk);
}

#endif
