#ifndef VOXEL_UTILS_H
#define VOXEL_UTILS_H

#include <optional>

#include "Coord.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"

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

	// Gets the voxel a point is in.
	VoxelInt3 pointToVoxel(const VoxelDouble3 &point);
	VoxelInt2 pointToVoxel(const VoxelDouble2 &point);

	// Converts a voxel from chunk space to new voxel grid space.
	NewDouble3 chunkPointToNewPoint(const ChunkInt2 &chunk, const VoxelDouble3 &point);
	NewDouble2 chunkPointToNewPoint(const ChunkInt2 &chunk, const VoxelDouble2 &point);
	NewInt3 chunkVoxelToNewVoxel(const ChunkInt2 &chunk, const VoxelInt3 &voxel);
	NewDouble3 coordToNewPoint(const CoordDouble3 &coord);
	NewDouble2 coordToNewPoint(const CoordDouble2 &coord);
	NewInt3 coordToNewVoxel(const CoordInt3 &coord);
	NewInt2 chunkVoxelToNewVoxel(const ChunkInt2 &chunk, const VoxelInt2 &voxel);

	// Converts a voxel from new voxel grid space to chunk voxel space.
	CoordDouble3 newPointToCoord(const NewDouble3 &point);
	CoordDouble2 newPointToCoord(const NewDouble2 &point);
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
