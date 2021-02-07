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
	void getChunkCounts(SNInt gridWidth, WEInt gridDepth, SNInt *outChunkCountX, WEInt *outChunkCountZ);

	// Gets the number of chunks that are potentially visible at any given time.
	void getPotentiallyVisibleChunkCounts(int chunkDistance, SNInt *outChunkCountX, WEInt *outChunkCountZ);

	// Gets chunk coordinates in an inclusive NxN pattern around the given chunk. Chunk distance is
	// the distance away from the given chunk in X or Z to reach (to obtain 3x3, 5x5, etc.).
	void getSurroundingChunks(const ChunkInt2 &chunk, int chunkDistance, ChunkInt2 *outMinChunk,
		ChunkInt2 *outMaxChunk);

	// Returns whether the given chunk at least partially touches the level dimensions. Intended for
	// interiors and cities.
	bool touchesLevelDimensions(const ChunkInt2 &chunk, SNInt levelWidth, WEInt levelDepth);

	// Returns whether the other chunk is close enough to the given chunk to be considered active.
	bool isWithinActiveRange(const ChunkInt2 &chunk, const ChunkInt2 &other, int chunkDistance);

	// Calculates a valid chunk+voxel pair from the given values. This is useful when stepping through
	// a chunk and the chunk edge may have been crossed (given by 'voxel' being outside the typical
	// [0, CHUNK_DIM-1] range), requiring a new chunk look-up.
	CoordDouble2 recalculateCoord(const ChunkInt2 &chunk, const VoxelDouble2 &point);
	CoordDouble3 recalculateCoord(const ChunkInt2 &chunk, const VoxelDouble3 &point);
	CoordInt2 recalculateCoord(const ChunkInt2 &chunk, const VoxelInt2 &voxel);
	CoordInt3 recalculateCoord(const ChunkInt2 &chunk, const VoxelInt3 &voxel);
}

#endif
