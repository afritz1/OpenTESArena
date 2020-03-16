#ifndef VOXEL_UTILS_H
#define VOXEL_UTILS_H

#include "../Math/Vector2.h"

// Aliases for various coordinate systems. All of these are from a top-down perspective, like a 2D array.
using OriginalInt2 = Int2; // +X west, +Y south (original game, origin at top right).
using NewInt2 = Int2; // +X north, +Y east (DEPRECATE THIS EVENTUALLY IN FAVOR OF CHUNKS).
using ChunkInt2 = Int2; // +X east, +Y south, [-inf, inf] (like a C array, origin at top left).
using ChunkVoxelInt2 = Int2; // Same directions as chunk, [0, CHUNK_DIM-1].
using AbsoluteChunkVoxelInt2 = Int2; // Chunk voxel multiplied by chunk coordinates, [-inf, inf].

struct ChunkCoord
{
	ChunkInt2 chunk;
	ChunkVoxelInt2 voxel;
};

// These are here out of desperation after many months of confusing myself.
using NSInt = int; // + north, - south
using SNInt = int; // + south, - north
using EWInt = int; // + east, - west
using WEInt = int; // + west, - east

namespace VoxelUtils
{
	// Number of voxels per side on a chunk.
	constexpr int CHUNK_DIM = 64;

	// I.e., given 23, returns 64.
	int getNextHigherChunkMultiple(int coord);

	// Gets the number of chunks in each dimension required to fit the given area that's in
	// new voxel grid space.
	void getChunkCounts(NSInt gridWidth, EWInt gridDepth, EWInt *outChunkCountX, SNInt *outChunkCountY);

	// Gets chunks in an NxN pattern around the given chunk. Useful for potentially visible chunk
	// coordinates around the camera position. Chunk distance is the distance away from the given
	// chunk in X or Y to reach (to obtain 3x3, 5x5, etc.).
	void getSurroundingChunks(const ChunkInt2 &chunk, int chunkDist, ChunkInt2 *outMinChunk,
		ChunkInt2 *outMaxChunk);

	// Transformation methods for converting voxel coordinates between the original game's format
	// (+X west, +Z south) and the new format (+X north, +Z east). This is a bi-directional
	// conversion (i.e., it works both ways. Not exactly sure why).
	NewInt2 originalVoxelToNewVoxel(const OriginalInt2 &voxel, NSInt gridWidth, EWInt gridDepth);
	OriginalInt2 newVoxelToOriginalVoxel(const NewInt2 &voxel, NSInt gridWidth, EWInt gridDepth);
	Double2 getTransformedVoxel(const Double2 &voxel, NSInt gridWidth, EWInt gridDepth);

	// Converts a voxel from chunk space to new voxel grid space.
	NewInt2 chunkVoxelToNewVoxel(const ChunkInt2 &chunk, const ChunkVoxelInt2 &voxel,
		NSInt gridWidth, EWInt gridDepth);

	// Converts a voxel from new voxel grid space to chunk voxel space.
	ChunkCoord newVoxelToChunkVoxel(const NewInt2 &voxel, NSInt gridWidth, EWInt gridDepth);

	// Gets the chunk that a new voxel would be in.
	ChunkInt2 newVoxelToChunk(const NewInt2 &voxel, NSInt gridWidth, EWInt gridDepth);
}

#endif
