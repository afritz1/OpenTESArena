#ifndef CHUNK_UTILS_H
#define CHUNK_UTILS_H

#include "../Voxels/VoxelUtils.h"

namespace ChunkUtils
{
	// Number of voxels per side on a chunk.
	constexpr int CHUNK_DIM = 64;

	// Smallest number of chunks away from the player allowed (1 results in a 3x3 grid).
	constexpr int MIN_CHUNK_DISTANCE = 1;

	// I.e., given 23, returns 64.
	int getNextHigherChunkMultiple(int coord);

	// Gets the side length (in chunks) of the active chunks for the given chunk distance.
	int getChunkCountPerSide(int chunkDistance);

	// Gets the number of chunks needed for the given chunk distance.
	int getChunkCount(int chunkDistance);

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

	// For iterating only the portion of a level that the chunk overlaps.
	void GetWritingRanges(const WorldInt2 &levelOffset, SNInt levelWidth, int levelHeight, WEInt levelDepth,
		SNInt *outStartX, int *outStartY, WEInt *outStartZ, SNInt *outEndX, int *outEndY, WEInt *outEndZ);

	bool IsInWritingRange(const WorldInt3 &position, SNInt startX, SNInt endX, int startY, int endY,
		WEInt startZ, WEInt endZ);

	VoxelInt3 MakeChunkVoxelFromLevel(const WorldInt3 &levelPosition, SNInt chunkStartX, int chunkStartY, WEInt chunkStartZ);
	VoxelInt2 MakeChunkVoxelFromLevel(const WorldInt2 &levelPosition, SNInt chunkStartX, WEInt chunkStartZ);
	VoxelDouble2 MakeChunkPointFromLevel(const WorldDouble2 &levelPosition, SNInt chunkStartX, WEInt chunkStartZ);
}

#endif
