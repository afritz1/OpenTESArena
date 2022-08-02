#include <cmath>

#include "ChunkUtils.h"
#include "VoxelFacing2D.h"
#include "VoxelFacing3D.h"
#include "VoxelUtils.h"

#include "components/debug/Debug.h"

NewInt2 VoxelUtils::originalVoxelToNewVoxel(const OriginalInt2 &voxel)
{
	return NewInt2(voxel.y, voxel.x);
}

OriginalInt2 VoxelUtils::newVoxelToOriginalVoxel(const NewInt2 &voxel)
{
	return VoxelUtils::originalVoxelToNewVoxel(voxel);
}

Double2 VoxelUtils::getTransformedVoxel(const Double2 &voxel)
{
	return Double2(voxel.y, voxel.x);
}

VoxelInt3 VoxelUtils::pointToVoxel(const VoxelDouble3 &point, double ceilingScale)
{
	return VoxelInt3(
		static_cast<SNInt>(std::floor(point.x)),
		static_cast<int>(std::floor(point.y / ceilingScale)),
		static_cast<WEInt>(std::floor(point.z)));
}

VoxelInt3 VoxelUtils::pointToVoxel(const VoxelDouble3 &point)
{
	constexpr double ceilingScale = 1.0;
	return VoxelUtils::pointToVoxel(point, ceilingScale);
}

VoxelInt2 VoxelUtils::pointToVoxel(const VoxelDouble2 &point)
{
	return VoxelInt2(
		static_cast<SNInt>(std::floor(point.x)),
		static_cast<WEInt>(std::floor(point.y)));
}

NewDouble3 VoxelUtils::chunkPointToNewPoint(const ChunkInt2 &chunk, const VoxelDouble3 &point)
{
	const NewDouble3 basePoint(
		static_cast<SNDouble>(chunk.x) * ChunkUtils::CHUNK_DIM,
		0.0,
		static_cast<WEDouble>(chunk.y) * ChunkUtils::CHUNK_DIM);
	return basePoint + point;
}

NewDouble2 VoxelUtils::chunkPointToNewPoint(const ChunkInt2 &chunk, const VoxelDouble2 &point)
{
	const NewDouble2 basePoint(
		static_cast<SNDouble>(chunk.x) * ChunkUtils::CHUNK_DIM,
		static_cast<WEDouble>(chunk.y) * ChunkUtils::CHUNK_DIM);
	return basePoint + point;
}

NewInt3 VoxelUtils::chunkVoxelToNewVoxel(const ChunkInt2 &chunk, const VoxelInt3 &voxel)
{
	const NewInt3 baseVoxel(chunk.x * ChunkUtils::CHUNK_DIM, 0, chunk.y * ChunkUtils::CHUNK_DIM);
	return baseVoxel + voxel;
}

NewDouble3 VoxelUtils::coordToNewPoint(const CoordDouble3 &coord)
{
	return VoxelUtils::chunkPointToNewPoint(coord.chunk, coord.point);
}

NewDouble2 VoxelUtils::coordToNewPoint(const CoordDouble2 &coord)
{
	return VoxelUtils::chunkPointToNewPoint(coord.chunk, coord.point);
}

NewInt3 VoxelUtils::coordToNewVoxel(const CoordInt3 &coord)
{
	return VoxelUtils::chunkVoxelToNewVoxel(coord.chunk, coord.voxel);
}

NewInt2 VoxelUtils::coordToNewVoxel(const CoordInt2 &coord)
{
	const NewInt3 voxel3D = VoxelUtils::chunkVoxelToNewVoxel(coord.chunk, VoxelInt3(coord.voxel.x, 0, coord.voxel.y));
	return NewInt2(voxel3D.x, voxel3D.z);
}

NewInt2 VoxelUtils::chunkVoxelToNewVoxel(const ChunkInt2 &chunk, const VoxelInt2 &voxel)
{
	return (chunk * ChunkUtils::CHUNK_DIM) + voxel;
}

CoordDouble3 VoxelUtils::newPointToCoord(const NewDouble3 &point)
{
	constexpr int chunkDim = ChunkUtils::CHUNK_DIM;
	constexpr double chunkDimReal = static_cast<double>(chunkDim);
	const ChunkInt2 chunk(
		((point.x >= 0.0) ? static_cast<SNInt>(point.x) : (static_cast<SNInt>(std::floor(point.x)) - (chunkDim - 1))) / chunkDim,
		((point.z >= 0.0) ? static_cast<WEInt>(point.z) : (static_cast<WEInt>(std::floor(point.z)) - (chunkDim - 1))) / chunkDim);
	const VoxelDouble3 newPoint(
		(point.x >= 0) ? std::fmod(point.x, chunkDimReal) : (chunkDimReal - std::fmod(-point.x, chunkDimReal)),
		(point.y >= 0) ? std::fmod(point.y, chunkDimReal) : (chunkDimReal - std::fmod(-point.y, chunkDimReal)),
		(point.z >= 0) ? std::fmod(point.z, chunkDimReal) : (chunkDimReal - std::fmod(-point.z, chunkDimReal)));
	return CoordDouble3(chunk, newPoint);
}

CoordDouble2 VoxelUtils::newPointToCoord(const NewDouble2 &point)
{
	constexpr int chunkDim = ChunkUtils::CHUNK_DIM;
	constexpr double chunkDimReal = static_cast<double>(chunkDim);
	const ChunkInt2 chunk(
		((point.x >= 0.0) ? static_cast<SNInt>(point.x) : (static_cast<SNInt>(std::floor(point.x)) - (chunkDim - 1))) / chunkDim,
		((point.y >= 0.0) ? static_cast<WEInt>(point.y) : (static_cast<WEInt>(std::floor(point.y)) - (chunkDim - 1))) / chunkDim);
	const VoxelDouble2 newPoint(
		(point.x >= 0) ? std::fmod(point.x, chunkDimReal) : (chunkDimReal - std::fmod(-point.x, chunkDimReal)),
		(point.y >= 0) ? std::fmod(point.y, chunkDimReal) : (chunkDimReal - std::fmod(-point.y, chunkDimReal)));
	return CoordDouble2(chunk, newPoint);
}

CoordInt3 VoxelUtils::newVoxelToCoord(const NewInt3 &voxel)
{
	constexpr int chunkDim = ChunkUtils::CHUNK_DIM;
	const ChunkInt2 chunk(
		((voxel.x >= 0) ? voxel.x : (voxel.x - (chunkDim - 1))) / chunkDim,
		((voxel.z >= 0) ? voxel.z : (voxel.z - (chunkDim - 1))) / chunkDim);
	const VoxelInt3 newVoxel(
		(voxel.x >= 0) ? (voxel.x % chunkDim) : (chunkDim - (-voxel.x % chunkDim)),
		(voxel.y >= 0) ? (voxel.y % chunkDim) : (chunkDim - (-voxel.y % chunkDim)),
		(voxel.z >= 0) ? (voxel.z % chunkDim) : (chunkDim - (-voxel.z % chunkDim)));
	return CoordInt3(chunk, newVoxel);
}

CoordInt2 VoxelUtils::newVoxelToCoord(const NewInt2 &voxel)
{
	constexpr int chunkDim = ChunkUtils::CHUNK_DIM;
	const ChunkInt2 chunk(
		((voxel.x >= 0) ? voxel.x : (voxel.x - (chunkDim - 1))) / chunkDim,
		((voxel.y >= 0) ? voxel.y : (voxel.y - (chunkDim - 1))) / chunkDim);
	const VoxelInt2 newVoxel(
		(voxel.x >= 0) ? (voxel.x % chunkDim) : (chunkDim - (-voxel.x % chunkDim)),
		(voxel.y >= 0) ? (voxel.y % chunkDim) : (chunkDim - (-voxel.y % chunkDim)));
	return CoordInt2(chunk, newVoxel);
}

CoordInt2 VoxelUtils::levelVoxelToCoord(const LevelInt2 &voxel)
{
	// @todo: make sure it handles negative coordinates.
	return VoxelUtils::newVoxelToCoord(voxel);
}

ChunkInt2 VoxelUtils::newVoxelToChunk(const NewInt2 &voxel)
{
	const CoordInt2 chunkCoord = VoxelUtils::newVoxelToCoord(voxel);
	return chunkCoord.chunk;
}

VoxelInt3 VoxelUtils::getAdjacentVoxelXZ(const VoxelInt3 &voxel, const VoxelInt2 &direction)
{
	DebugAssert(std::abs(direction.x) <= 1);
	DebugAssert(std::abs(direction.y) <= 1);
	const VoxelInt3 diff(direction.x, 0, direction.y);
	return voxel + diff;
}

CoordInt3 VoxelUtils::getAdjacentCoordXZ(const CoordInt3 &coord, const VoxelInt2 &direction)
{
	return ChunkUtils::recalculateCoord(coord.chunk, VoxelUtils::getAdjacentVoxelXZ(coord.voxel, direction));
}

VoxelInt2 VoxelUtils::wrapVoxelCoord(const VoxelInt2 &voxel)
{
	// @todo: handle negative numbers
	return VoxelInt2(voxel.x % ChunkUtils::CHUNK_DIM, voxel.y % ChunkUtils::CHUNK_DIM);
}

Double3 VoxelUtils::getVoxelCenter(const Int3 &voxel, double ceilingScale)
{
	return Double3(
		static_cast<double>(voxel.x) + 0.50,
		(static_cast<double>(voxel.y) + 0.50) * ceilingScale,
		static_cast<double>(voxel.z) + 0.50);
}

Double3 VoxelUtils::getVoxelCenter(const Int3 &voxel)
{
	constexpr double ceilingScale = 1.0;
	return VoxelUtils::getVoxelCenter(voxel, ceilingScale);
}

Double2 VoxelUtils::getVoxelCenter(const Int2 &voxel)
{
	return Double2(
		static_cast<double>(voxel.x) + 0.50,
		static_cast<double>(voxel.y) + 0.50);
}

Double3 VoxelUtils::getNormal(VoxelFacing2D facing)
{
	if (facing == VoxelFacing2D::PositiveX)
	{
		return Double3::UnitX;
	}
	else if (facing == VoxelFacing2D::NegativeX)
	{
		return -Double3::UnitX;
	}
	else if (facing == VoxelFacing2D::PositiveZ)
	{
		return Double3::UnitZ;
	}
	else if (facing == VoxelFacing2D::NegativeZ)
	{
		return -Double3::UnitZ;
	}
	else
	{
		DebugUnhandledReturnMsg(Double3, std::to_string(static_cast<int>(facing)));
	}
}

VoxelFacing3D VoxelUtils::convertFaceTo3D(VoxelFacing2D facing)
{
	if (facing == VoxelFacing2D::PositiveX)
	{
		return VoxelFacing3D::PositiveX;
	}
	else if (facing == VoxelFacing2D::NegativeX)
	{
		return VoxelFacing3D::NegativeX;
	}
	else if (facing == VoxelFacing2D::PositiveZ)
	{
		return VoxelFacing3D::PositiveZ;
	}
	else if (facing == VoxelFacing2D::NegativeZ)
	{
		return VoxelFacing3D::NegativeZ;
	}
	else
	{
		DebugUnhandledReturnMsg(VoxelFacing3D, std::to_string(static_cast<int>(facing)));
	}
}

std::optional<VoxelFacing2D> VoxelUtils::tryConvertFaceTo2D(VoxelFacing3D facing)
{
	if (facing == VoxelFacing3D::PositiveX)
	{
		return VoxelFacing2D::PositiveX;
	}
	else if (facing == VoxelFacing3D::NegativeX)
	{
		return VoxelFacing2D::NegativeX;
	}
	else if (facing == VoxelFacing3D::PositiveZ)
	{
		return VoxelFacing2D::PositiveZ;
	}
	else if (facing == VoxelFacing3D::NegativeZ)
	{
		return VoxelFacing2D::NegativeZ;
	}
	else if ((facing == VoxelFacing3D::PositiveY) || (facing == VoxelFacing3D::NegativeY))
	{
		return std::nullopt;
	}
	else
	{
		DebugUnhandledReturnMsg(VoxelFacing2D, std::to_string(static_cast<int>(facing)));
	}
}

void VoxelUtils::getSurroundingVoxels(const VoxelInt3 &voxel, int distance, VoxelInt3 *outMinVoxel, VoxelInt3 *outMaxVoxel)
{
	DebugAssert(distance >= 0);
	*outMinVoxel = VoxelInt3(voxel.x - distance, voxel.y - distance, voxel.z - distance);
	*outMaxVoxel = VoxelInt3(voxel.x + distance, voxel.y + distance, voxel.z + distance);
}

void VoxelUtils::getSurroundingVoxels(const VoxelInt2 &voxel, int distance, VoxelInt2 *outMinVoxel, VoxelInt2 *outMaxVoxel)
{
	DebugAssert(distance >= 0);
	*outMinVoxel = VoxelInt2(voxel.x - distance, voxel.y - distance);
	*outMaxVoxel = VoxelInt2(voxel.x + distance, voxel.y + distance);
}
