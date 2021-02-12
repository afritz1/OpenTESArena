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

VoxelInt3 VoxelUtils::pointToVoxel(const VoxelDouble3 &point)
{
	return VoxelInt3(
		static_cast<SNInt>(std::floor(point.x)),
		static_cast<int>(std::floor(point.y)),
		static_cast<WEInt>(std::floor(point.z)));
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

NewInt2 VoxelUtils::chunkVoxelToNewVoxel(const ChunkInt2 &chunk, const VoxelInt2 &voxel)
{
	return (chunk * ChunkUtils::CHUNK_DIM) + voxel;
}

CoordDouble3 VoxelUtils::newPointToCoord(const NewDouble3 &point)
{
	const ChunkInt2 chunk(
		static_cast<SNInt>(point.x) / ChunkUtils::CHUNK_DIM,
		static_cast<WEInt>(point.z) / ChunkUtils::CHUNK_DIM);
	const VoxelDouble3 voxel(
		std::fmod(point.x, static_cast<SNDouble>(ChunkUtils::CHUNK_DIM)),
		point.y,
		std::fmod(point.z, static_cast<WEDouble>(ChunkUtils::CHUNK_DIM)));
	return CoordDouble3(chunk, voxel);
}

CoordDouble2 VoxelUtils::newPointToCoord(const NewDouble2 &point)
{
	const ChunkInt2 chunk(
		static_cast<SNInt>(point.x) / ChunkUtils::CHUNK_DIM,
		static_cast<WEInt>(point.y) / ChunkUtils::CHUNK_DIM);
	const VoxelDouble2 voxel(
		std::fmod(point.x, static_cast<SNDouble>(ChunkUtils::CHUNK_DIM)),
		std::fmod(point.y, static_cast<WEDouble>(ChunkUtils::CHUNK_DIM)));
	return CoordDouble2(chunk, voxel);
}

CoordInt3 VoxelUtils::newVoxelToCoord(const NewInt3 &voxel)
{
	CoordInt3 coord(
		ChunkInt2(voxel.x / ChunkUtils::CHUNK_DIM, voxel.z / ChunkUtils::CHUNK_DIM),
		VoxelInt3(voxel.x % ChunkUtils::CHUNK_DIM, voxel.y, voxel.z % ChunkUtils::CHUNK_DIM));
	return coord;
}

CoordInt2 VoxelUtils::newVoxelToCoord(const NewInt2 &voxel)
{
	// @todo: need to handle voxel outside grid.
	// @todo: probably want (int)Floor() instead of modulo.

	CoordInt2 coord(
		ChunkInt2(voxel.x / ChunkUtils::CHUNK_DIM, voxel.y / ChunkUtils::CHUNK_DIM),
		VoxelInt2(voxel.x % ChunkUtils::CHUNK_DIM, voxel.y % ChunkUtils::CHUNK_DIM));
	return coord;
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

VoxelInt2 VoxelUtils::wrapVoxelCoord(const VoxelInt2 &voxel)
{
	// @todo: handle negative numbers
	return VoxelInt2(voxel.x % ChunkUtils::CHUNK_DIM, voxel.y % ChunkUtils::CHUNK_DIM);
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
