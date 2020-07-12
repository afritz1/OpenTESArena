#ifndef VOXEL_UTILS_H
#define VOXEL_UTILS_H

#include "../Math/Vector2.h"

// Aliases for various coordinate systems. All of these are from a top-down perspective.
using OriginalInt2 = Int2; // +X west, +Y south (original game, origin at top right).
using NewInt2 = Int2; // +X south, +Y west (DEPRECATE THIS EVENTUALLY IN FAVOR OF ChunkInt2 + VoxelInt2).
using ChunkInt2 = Int2; // +X south, +Y west, [-inf, inf].
using VoxelInt2 = Int2; // +X south, +Y west, used with chunk voxels, [0, CHUNK_DIM-1].

using OriginalDouble2 = Double2; // +X west, +Y south.
using NewDouble2 = Double2; // +X south, +Y west (DEPRECATE IN FAVOR OF VoxelDouble2).
using VoxelDouble2 = Double2; // +X south, +Y west, in the space of chunk voxels.

struct ChunkCoord
{
	ChunkInt2 chunk;
	VoxelInt2 voxel;
};

// These are here out of desperation after many months of confusing myself.
//using NSInt = int; // + north, - south
using SNInt = int; // + south, - north
//using EWInt = int; // + east, - west
using WEInt = int; // + west, - east
//using NSDouble = double; // + north, - south
using SNDouble = double; // + south, - north
//using EWDouble = double; // + east, - west
using WEDouble = double; // + west, - east

namespace VoxelUtils
{
	const NewInt2 North(-1, 0);
	const NewInt2 South(1, 0);
	const NewInt2 East(0, -1);
	const NewInt2 West(0, 1);

	// Transformation methods for converting voxel coordinates between the original game's format
	// (+X west, +Z south) and the new format (+X south, +Z west). This is a bi-directional
	// conversion (i.e., it works both ways).
	NewInt2 originalVoxelToNewVoxel(const OriginalInt2 &voxel);
	OriginalInt2 newVoxelToOriginalVoxel(const NewInt2 &voxel);
	Double2 getTransformedVoxel(const Double2 &voxel);

	// Converts a voxel from chunk space to new voxel grid space.
	NewInt2 chunkVoxelToNewVoxel(const ChunkInt2 &chunk, const VoxelInt2 &voxel);

	// Converts a voxel from new voxel grid space to chunk voxel space.
	ChunkCoord newVoxelToChunkVoxel(const NewInt2 &voxel);

	// Gets the chunk that a new voxel would be in.
	ChunkInt2 newVoxelToChunk(const NewInt2 &voxel);
}

#endif
