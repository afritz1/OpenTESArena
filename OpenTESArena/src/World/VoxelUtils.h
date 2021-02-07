#ifndef VOXEL_UTILS_H
#define VOXEL_UTILS_H

#include <optional>

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

enum class VoxelFacing2D;
enum class VoxelFacing3D;

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
	NewDouble3 chunkPointToNewPoint(const ChunkInt2 &chunk, const VoxelDouble3 &point);
	NewInt3 chunkVoxelToNewVoxel(const ChunkInt2 &chunk, const VoxelInt3 &voxel);
	NewDouble3 coordToNewPoint(const CoordDouble3 &coord);
	NewInt3 coordToNewVoxel(const CoordInt3 &coord);
	NewInt2 chunkVoxelToNewVoxel(const ChunkInt2 &chunk, const VoxelInt2 &voxel);

	// Converts a voxel from new voxel grid space to chunk voxel space.
	CoordDouble3 newPointToCoord(const NewDouble3 &point);
	CoordInt3 newVoxelToCoord(const NewInt3 &voxel);
	CoordInt2 newVoxelToCoord(const NewInt2 &voxel);

	// Converts a voxel from level definition space to chunk voxel space.
	CoordInt2 levelVoxelToCoord(const LevelInt2 &voxel);

	// Gets the chunk that a new voxel would be in.
	ChunkInt2 newVoxelToChunk(const NewInt2 &voxel);

	// Wraps a voxel coordinate so it stays within the chunk range.
	VoxelInt2 wrapVoxelCoord(const VoxelInt2 &voxel);

	// Adds half of a voxel to the voxel coordinate to get its center point.
	Double2 getVoxelCenter(const Int2 &voxel);

	// Gets the normal associated with a voxel facing.
	Double3 getNormal(VoxelFacing2D facing);

	// Converts between 2D and 3D specializations of voxel facings.
	VoxelFacing3D convertFaceTo3D(VoxelFacing2D facing);
	std::optional<VoxelFacing2D> tryConvertFaceTo2D(VoxelFacing3D facing);

	// Gets voxel coordinates in an inclusive NxN pattern around the given voxel. 'Distance' is
	// the number of voxels away from the given voxel to reach (to obtain 3x3, 5x5, etc.). Does not
	// clamp within any specified range.
	void getSurroundingVoxels(const VoxelInt3 &voxel, int distance, VoxelInt3 *outMinVoxel, VoxelInt3 *outMaxVoxel);
	void getSurroundingVoxels(const VoxelInt2 &voxel, int distance, VoxelInt2 *outMinVoxel, VoxelInt2 *outMaxVoxel);
}

#endif
