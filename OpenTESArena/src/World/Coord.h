#ifndef COORD_H
#define COORD_H

#include "../Math/Vector2.h"
#include "../Math/Vector3.h"

// Aliases for various coordinate systems. All of these are from a top-down perspective.
using OriginalInt2 = Int2; // +X west, +Y south (original game, origin at top right).
using WorldInt2 = Int2; // +X south, +Y west, relative to world origin, independent of chunks.
using ChunkInt2 = Int2; // +X south, +Y west, [-inf, inf].
using VoxelInt2 = Int2; // +X south, +Y west, used with chunk voxels, [0, CHUNK_DIM-1].

using OriginalDouble2 = Double2; // +X west, +Y south.
using WorldDouble2 = Double2; // +X south, +Y west, relative to world origin, independent of chunks.
using VoxelDouble2 = Double2; // +X south, +Y west, in the space of chunk voxels.

using WorldInt3 = Int3; // +X south, +Y up, +Z west, relative to world origin, independent of chunks.
using VoxelInt3 = Int3; // +X south, +Y up, +Z west, used with chunk voxels, [0, CHUNK_DIM-1].

using WorldDouble3 = Double3; // +X south, +Y up, +Z west, relative to world origin, independent of chunks.
using VoxelDouble3 = Double3; // +X south, +Y up, +Z west, used with chunk voxels, [0, CHUNK_DIM-1].

struct CoordDouble2;
struct CoordDouble3;

// Various coordinate structs for representing a unique voxel or point in the game world.
struct CoordInt2
{
	ChunkInt2 chunk;
	VoxelInt2 voxel;

	CoordInt2() = default;
	CoordInt2(const ChunkInt2 &chunk, const VoxelInt2 &voxel)
		: chunk(chunk), voxel(voxel) { }

	bool operator==(const CoordInt2 &other) const;
	bool operator!=(const CoordInt2 &other) const;

	CoordDouble2 toVoxelCenter() const;
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

	CoordInt2 toVoxel() const;
};

struct CoordInt3
{
	ChunkInt2 chunk;
	VoxelInt3 voxel;

	CoordInt3() = default;
	CoordInt3(const ChunkInt2 &chunk, const VoxelInt3 &voxel)
		: chunk(chunk), voxel(voxel) { }

	bool operator==(const CoordInt3 &other) const;
	bool operator!=(const CoordInt3 &other) const;
	CoordInt3 operator+(const VoxelInt3 &other) const;
	VoxelInt3 operator-(const CoordInt3 &other) const;

	CoordDouble3 toVoxelCenterScaled(double ceilingScale) const;
	CoordDouble3 toVoxelCenter() const;
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

	CoordInt3 toVoxelScaled(double ceilingScale) const;
	CoordInt3 toVoxel() const;
};

// These are here out of desperation after many months of confusing myself.
using SNInt = int; // + south, - north
using WEInt = int; // + west, - east
using SNDouble = double; // + south, - north
using WEDouble = double; // + west, - east

#endif
