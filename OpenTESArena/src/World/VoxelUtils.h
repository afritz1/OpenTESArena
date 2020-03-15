#ifndef VOXEL_UTILS_H
#define VOXEL_UTILS_H

#include "../Math/Vector2.h"

// Aliases for various coordinate systems. All of these are from a top-down perspective, like a 2D array.
using OriginalInt2 = Int2; // +X west, +Y south (original game, origin at top right).
using NewInt2 = Int2; // +X north, +Y east (DEPRECATE THIS EVENTUALLY IN FAVOR OF CHUNKS).
using ChunkInt2 = Int2; // +X east, +Y south, [-inf, inf] (like a C array, origin at top left).
using ChunkVoxelInt2 = Int2; // Same directions as chunk, [0, CHUNK_DIM-1].

// These are here out of desperation after many months of confusing myself.
using NSInt = int; // + north, - south
using SNInt = int; // + south, - north
using EWInt = int; // + east, - west
using WEInt = int; // + west, - east

namespace VoxelUtils
{
	// Number of voxels per side on a chunk.
	constexpr int CHUNK_DIM = 64;

	// @todo: move transform function out of VoxelGrid?

	// Converts a voxel from chunk space to new voxel grid space.
	NewInt2 chunkVoxelToNewVoxel(const ChunkInt2 &chunk, const ChunkVoxelInt2 &voxel,
		NSInt gridWidth, EWInt gridDepth);
}

#endif
