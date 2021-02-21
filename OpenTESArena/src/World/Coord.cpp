#include "ChunkUtils.h"
#include "Coord.h"

bool CoordInt2::operator==(const CoordInt2 &other) const
{
	return (this->chunk == other.chunk) && (this->voxel == other.voxel);
}

bool CoordInt2::operator!=(const CoordInt2 &other) const
{
	return (this->chunk != other.chunk) || (this->voxel != other.voxel);
}

CoordDouble2 CoordDouble2::operator+(const VoxelDouble2 &other) const
{
	return CoordDouble2(this->chunk, this->point + other);
}

CoordDouble2 CoordDouble2::operator-(const VoxelDouble2 &other) const
{
	return CoordDouble2(this->chunk, this->point - other);
}

VoxelDouble2 CoordDouble2::operator-(const CoordDouble2 &other) const
{
	// Combine three vectors:
	// 1) Other chunk point to other chunk origin.
	// 2) Other chunk origin to local chunk origin.
	// 3) Local chunk origin to local point.
	const VoxelDouble2 otherPointToOtherOrigin = -other.point;

	const ChunkInt2 chunkDiff = this->chunk - other.chunk;
	const VoxelDouble2 otherOriginToOrigin(
		chunkDiff.x * static_cast<SNDouble>(ChunkUtils::CHUNK_DIM),
		chunkDiff.y * static_cast<WEDouble>(ChunkUtils::CHUNK_DIM));

	const VoxelDouble2 originToPoint = this->point;

	return otherPointToOtherOrigin + otherOriginToOrigin + originToPoint;
}

bool CoordInt3::operator==(const CoordInt3 &other) const
{
	return (this->chunk == other.chunk) && (this->voxel == other.voxel);
}

bool CoordInt3::operator!=(const CoordInt3 &other) const
{
	return (this->chunk != other.chunk) || (this->voxel != other.voxel);
}

CoordInt3 CoordInt3::operator+(const VoxelInt3 &other) const
{
	return ChunkUtils::recalculateCoord(this->chunk, this->voxel + other);
}

VoxelInt3 CoordInt3::operator-(const CoordInt3 &other) const
{
	// Combine three vectors:
	// 1) Other chunk point to other chunk origin.
	// 2) Other chunk origin to local chunk origin.
	// 3) Local chunk origin to local point.
	const VoxelInt3 otherPointToOtherOrigin = -other.voxel;

	const ChunkInt2 chunkDiff = this->chunk - other.chunk;
	const VoxelInt3 otherOriginToOrigin(
		chunkDiff.x * ChunkUtils::CHUNK_DIM,
		0.0,
		chunkDiff.y * ChunkUtils::CHUNK_DIM);

	const VoxelInt3 originToPoint = this->voxel;

	return otherPointToOtherOrigin + otherOriginToOrigin + originToPoint;
}

CoordDouble3 CoordDouble3::operator+(const VoxelDouble3 &other) const
{
	return ChunkUtils::recalculateCoord(this->chunk, this->point + other);
}

VoxelDouble3 CoordDouble3::operator-(const CoordDouble3 &other) const
{
	// Combine three vectors:
	// 1) Other chunk point to other chunk origin.
	// 2) Other chunk origin to local chunk origin.
	// 3) Local chunk origin to local point.
	const VoxelDouble3 otherPointToOtherOrigin = -other.point;

	const ChunkInt2 chunkDiff = this->chunk - other.chunk;
	const VoxelDouble3 otherOriginToOrigin(
		chunkDiff.x * static_cast<SNDouble>(ChunkUtils::CHUNK_DIM),
		0.0,
		chunkDiff.y * static_cast<WEDouble>(ChunkUtils::CHUNK_DIM));

	const VoxelDouble3 originToPoint = this->point;

	return otherPointToOtherOrigin + otherOriginToOrigin + originToPoint;
}
