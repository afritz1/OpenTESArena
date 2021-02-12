#include "ChunkUtils.h"
#include "Coord.h"

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

CoordDouble3 CoordDouble3::operator+(const VoxelDouble3 &other) const
{
	return CoordDouble3(this->chunk, this->point + other);
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
