#ifndef VOXEL_UTILS_H
#define VOXEL_UTILS_H

#include <optional>

#include "VoxelFacing3D.h"
#include "../Assets/ArenaTypes.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../World/Coord.h"

#include "components/utilities/BufferView.h"

enum class VoxelFacing2D;

namespace VoxelUtils
{
	static constexpr int FACE_COUNT = 6; // +X, -X, +Y, -Y, +Z, -Z

	const VoxelInt2 North(-1, 0);
	const VoxelInt2 South(1, 0);
	const VoxelInt2 East(0, -1);
	const VoxelInt2 West(0, 1);

	// Transformation methods for converting voxel coordinates between the original game's format
	// (+X west, +Z south) and the new format (+X south, +Z west). This is a bi-directional
	// conversion (i.e., it works both ways).
	WorldInt2 originalVoxelToWorldVoxel(const OriginalInt2 &voxel);
	OriginalInt2 worldVoxelToOriginalVoxel(const WorldInt2 &voxel);
	Double2 getTransformedVoxel(const Double2 &voxel);

	// Gets the voxel a point is in. Requires wall height if including Y.
	VoxelInt3 pointToVoxel(const VoxelDouble3 &point, double ceilingScale);
	VoxelInt2 pointToVoxel(const VoxelDouble2 &point);

	// Converts a voxel from chunk space to new voxel grid space.
	WorldDouble3 chunkPointToWorldPoint(const ChunkInt2 &chunk, const VoxelDouble3 &point);
	WorldDouble2 chunkPointToWorldPoint(const ChunkInt2 &chunk, const VoxelDouble2 &point);
	WorldInt3 chunkVoxelToWorldVoxel(const ChunkInt2 &chunk, const VoxelInt3 &voxel);
	WorldDouble3 coordToWorldPoint(const CoordDouble3 &coord);
	WorldDouble2 coordToWorldPoint(const CoordDouble2 &coord);
	WorldInt3 coordToWorldVoxel(const CoordInt3 &coord);
	WorldInt2 coordToWorldVoxel(const CoordInt2 &coord);
	WorldInt2 chunkVoxelToWorldVoxel(const ChunkInt2 &chunk, const VoxelInt2 &voxel);

	// Converts from world space to chunk space.
	ChunkInt2 worldPointToChunk(const WorldDouble3 &point);
	ChunkInt2 worldPointToChunk(const WorldDouble2 &point);
	CoordDouble3 worldPointToCoord(const WorldDouble3 &point);
	CoordDouble2 worldPointToCoord(const WorldDouble2 &point);
	ChunkInt2 worldVoxelToChunk(const WorldInt3 &voxel);
	ChunkInt2 worldVoxelToChunk(const WorldInt2 &voxel);
	CoordInt3 worldVoxelToCoord(const WorldInt3 &voxel);
	CoordInt2 worldVoxelToCoord(const WorldInt2 &voxel);

	// Converts a voxel from level definition space to chunk voxel space.
	CoordInt2 levelVoxelToCoord(const WorldInt2 &voxel);

	// Gets the coordinate of an adjacent voxel.
	VoxelInt3 getAdjacentVoxelXZ(const VoxelInt3 &voxel, const VoxelInt2 &direction);
	CoordInt3 getAdjacentCoordXZ(const CoordInt3 &coord, const VoxelInt2 &direction);

	// Wraps a voxel coordinate so it stays within the chunk range.
	VoxelInt2 wrapVoxelCoord(const VoxelInt2 &voxel);

	// Adds half of a voxel to the voxel coordinate to get its center point.
	Double3 getVoxelCenter(const Int3 &voxel, double ceilingScale);
	Double3 getVoxelCenter(const Int3 &voxel);
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

	// Valid facings for face enabling/combining logic.
	constexpr VoxelFacing3D ValidFacings_Wall[] = { VoxelFacing3D::PositiveX, VoxelFacing3D::NegativeX, VoxelFacing3D::PositiveY, VoxelFacing3D::NegativeY, VoxelFacing3D::PositiveZ, VoxelFacing3D::NegativeZ };
	constexpr VoxelFacing3D ValidFacings_Floor[] = { VoxelFacing3D::PositiveY };
	constexpr VoxelFacing3D ValidFacings_Ceiling[] = { VoxelFacing3D::NegativeY };
	constexpr VoxelFacing3D ValidFacings_Raised[] = { VoxelFacing3D::PositiveX, VoxelFacing3D::NegativeX, VoxelFacing3D::PositiveY, VoxelFacing3D::NegativeY, VoxelFacing3D::PositiveZ, VoxelFacing3D::NegativeZ };
	constexpr VoxelFacing3D ValidFacings_TransparentWall[] = { VoxelFacing3D::PositiveX, VoxelFacing3D::NegativeX, VoxelFacing3D::PositiveZ, VoxelFacing3D::NegativeZ };

	const std::pair<ArenaVoxelType, BufferView<const VoxelFacing3D>> VoxelTypeValidFacings[] =
	{
		{ ArenaVoxelType::None, BufferView<const VoxelFacing3D>() },
		{ ArenaVoxelType::Wall, ValidFacings_Wall },
		{ ArenaVoxelType::Floor, ValidFacings_Floor },
		{ ArenaVoxelType::Ceiling, ValidFacings_Ceiling },
		{ ArenaVoxelType::Raised, ValidFacings_Raised },
		{ ArenaVoxelType::Diagonal, BufferView<const VoxelFacing3D>() }, // Needs more than facing check
		{ ArenaVoxelType::TransparentWall, ValidFacings_TransparentWall },
		{ ArenaVoxelType::Edge, BufferView<const VoxelFacing3D>() }, // Depends on edge definition
		{ ArenaVoxelType::Chasm, BufferView<const VoxelFacing3D>() }, // Depends on chasm wall instance
		{ ArenaVoxelType::Door, BufferView<const VoxelFacing3D>() } // Not worth combining
	};

	int getFacingIndex(VoxelFacing3D facing);
	VoxelFacing3D getFaceIndexFacing(int faceIndex);
	VoxelFacing3D getOppositeFacing(VoxelFacing3D facing);
}

#endif
