#ifndef COORD_H
#define COORD_H

#include "../Math/Vector2.h"
#include "../Math/Vector3.h"

// Aliases for various coordinate systems. All of these are from a top-down perspective.
using OriginalInt2 = Int2; // +X west, +Y south (original game, origin at top right).
using NewInt2 = Int2; // +X south, +Y west (DEPRECATE THIS EVENTUALLY IN FAVOR OF ChunkInt2 + VoxelInt2).
using LevelInt2 = Int2; // +X south, +Y west, used with level definitions (independent of chunks).
using ChunkInt2 = Int2; // +X south, +Y west, [-inf, inf].
using VoxelInt2 = Int2; // +X south, +Y west, used with chunk voxels, [0, CHUNK_DIM-1].

using OriginalDouble2 = Double2; // +X west, +Y south.
using NewDouble2 = Double2; // +X south, +Y west (DEPRECATE IN FAVOR OF VoxelDouble2).
using LevelDouble2 = Double2; // +X south, +Y west, used with level definitions (independent of chunks).
using VoxelDouble2 = Double2; // +X south, +Y west, in the space of chunk voxels.

using NewInt3 = Int3; // +X south, +Y up, +Z west (DEPRECATE THIS EVENTUALLY IN FAVOR OF ChunkInt2 + VoxelInt3).
using LevelInt3 = Int3; // +X south, +Y up, +Z west, used with level definitions (independent of chunks).
using VoxelInt3 = Int3; // +X south, +Y up, +Z west, used with chunk voxels, [0, CHUNK_DIM-1].

using NewDouble3 = Double3; // +X south, +Y up, +Z west (DEPRECATE THIS EVENTUALLY IN FAVOR OF ChunkInt2 + VoxelInt3).
using LevelDouble3 = Double3; // +X south, +Y up, +Z west, used with level definitions (independent of chunks).
using VoxelDouble3 = Double3; // +X south, +Y up, +Z west, used with chunk voxels, [0, CHUNK_DIM-1].

// Various coordinate structs for representing a unique voxel or point in the game world.
struct CoordInt2
{
	ChunkInt2 chunk;
	VoxelInt2 voxel;

	CoordInt2() = default;
	CoordInt2(const ChunkInt2 &chunk, const VoxelInt2 &voxel)
		: chunk(chunk), voxel(voxel) { }
};

struct CoordDouble2
{
	ChunkInt2 chunk;
	VoxelDouble2 point;

	CoordDouble2() = default;
	CoordDouble2(const ChunkInt2 &chunk, const VoxelDouble2 &point)
		: chunk(chunk), point(point) { }

	CoordDouble2 operator+(const VoxelDouble2 &other) const;
	CoordDouble2 operator-(const VoxelDouble2 &other) const;
	VoxelDouble2 operator-(const CoordDouble2 &other) const;
};

struct CoordInt3
{
	ChunkInt2 chunk;
	VoxelInt3 voxel;

	CoordInt3() = default;
	CoordInt3(const ChunkInt2 &chunk, const VoxelInt3 &voxel)
		: chunk(chunk), voxel(voxel) { }
};

struct CoordDouble3
{
	ChunkInt2 chunk;
	VoxelDouble3 point;

	CoordDouble3() = default;
	CoordDouble3(const ChunkInt2 &chunk, const VoxelDouble3 &point)
		: chunk(chunk), point(point) { }

	CoordDouble3 operator+(const VoxelDouble3 &other) const;
	VoxelDouble3 operator-(const CoordDouble3 &other) const;
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

#endif
