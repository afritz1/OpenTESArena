#include "ArenaMeshUtils.h"
#include "MeshLibrary.h"
#include "MapType.h"
#include "../Math/Constants.h"
#include "../Player/ArenaPlayerUtils.h"
#include "../Voxels/VoxelFacing.h"

ArenaShapeRaisedInitInfo::ArenaShapeRaisedInitInfo()
{
	this->vTop = 0.0;
	this->vBottom = 0.0;
}

ArenaShapeEdgeInitInfo::ArenaShapeEdgeInitInfo()
{
	this->facing = static_cast<VoxelFacing2D>(-1);
	this->flippedTexCoords = false;
}

ArenaShapeDiagonalInitInfo::ArenaShapeDiagonalInitInfo()
{
	this->isRightDiagonal = false;
}

ArenaShapeInitCache::ArenaShapeInitCache()
{
	this->boxWidth = 0.0;
	this->boxHeight = 0.0;
	this->boxDepth = 0.0;
	this->boxYOffset = 0.0;
	this->boxYRotation = 0.0;
	this->voxelType = static_cast<ArenaVoxelType>(-1);
}

void ArenaShapeInitCache::initDefaultBoxValues(ArenaVoxelType voxelType)
{
	this->boxWidth = 1.0;
	this->boxHeight = 1.0;
	this->boxDepth = 1.0;
	this->boxYOffset = 0.0;
	this->boxYRotation = 0.0;
	this->voxelType = voxelType;
}

void ArenaShapeInitCache::initRaisedBoxValues(double yOffset, double ySize, double vTop, double vBottom)
{
	this->boxWidth = 1.0;
	this->boxHeight = ySize;
	this->boxDepth = 1.0;
	this->boxYOffset = yOffset;
	this->boxYRotation = 0.0;
	this->voxelType = ArenaVoxelType::Raised;

	this->raised.vTop = vTop;
	this->raised.vBottom = vBottom;
}

void ArenaShapeInitCache::initEdgeBoxValues(double yOffset, VoxelFacing2D facing, bool flippedTexCoords)
{
	this->boxWidth = 1.0;
	this->boxHeight = 1.0;
	this->boxDepth = 1.0;
	this->boxYOffset = yOffset;
	this->boxYRotation = 0.0;
	this->voxelType = ArenaVoxelType::Edge;

	this->edge.facing = facing;
	this->edge.flippedTexCoords = flippedTexCoords;
}

void ArenaShapeInitCache::initChasmBoxValues(bool isDryChasm, MapType mapType)
{
	this->boxWidth = 1.0;
	this->boxHeight = 1.0;
	this->boxDepth = 1.0;
	
	this->boxYOffset = -1.0;
	if (!isDryChasm)
	{
		// @todo this was a guess-and-checked value that looks good enough
		// - probably some mix of INFCeiling::DEFAULT_HEIGHT, MIFUtils::ARENA_UNITS, and ArenaPlayerUtils::EyeHeight
		constexpr int unknownOffset = -83;

		int cameraOffset = unknownOffset;
		switch (mapType)
		{
		case MapType::Interior:
			cameraOffset += ArenaPlayerUtils::ChasmHeightSwimmingInterior;
			break;
		case MapType::City:
			cameraOffset += ArenaPlayerUtils::ChasmHeightSwimmingCity;
			break;
		case MapType::Wilderness:
			cameraOffset += ArenaPlayerUtils::ChasmHeightSwimmingWild;
			break;
		default:
			DebugNotImplemented();
			break;
		}

		this->boxYOffset = static_cast<double>(cameraOffset) / MIFUtils::ARENA_UNITS;
	}

	this->boxYRotation = 0.0;
	this->voxelType = ArenaVoxelType::Chasm;
}

void ArenaShapeInitCache::initDiagonalBoxValues(bool isRightDiagonal)
{
	constexpr Radians diagonalAngle = Constants::Pi / 4.0;
	constexpr double diagonalThickness = 0.050; // Arbitrary thin wall thickness
	static_assert(diagonalThickness > (Physics::BoxConvexRadius * 2.0));

	this->boxWidth = Constants::Sqrt2 - diagonalThickness; // Fit the edges of the voxel exactly
	this->boxHeight = 1.0;
	this->boxDepth = diagonalThickness;
	this->boxYOffset = 0.0;
	this->boxYRotation = isRightDiagonal ? -diagonalAngle : diagonalAngle;
	this->voxelType = ArenaVoxelType::Diagonal;

	this->diagonal.isRightDiagonal = isRightDiagonal;
}

bool ArenaMeshUtils::isFullyCoveringFacing(const ArenaShapeInitCache &shapeInitCache, VoxelFacing3D facing)
{
	const ArenaVoxelType voxelType = shapeInitCache.voxelType;

	switch (voxelType)
	{
	case ArenaVoxelType::None:
		return false;
	case ArenaVoxelType::Wall:
		return true;
	case ArenaVoxelType::Floor:
		return facing == VoxelFacing3D::PositiveY;
	case ArenaVoxelType::Ceiling:
		return facing == VoxelFacing3D::NegativeY;
	case ArenaVoxelType::Raised:
	{
		if (facing == VoxelFacing3D::PositiveY)
		{
			return MathUtils::almostEqual(shapeInitCache.boxYOffset + shapeInitCache.boxHeight, 1.0);
		}
		else if (facing == VoxelFacing3D::NegativeY)
		{
			return shapeInitCache.boxYOffset == 0.0;
		}
		else
		{
			return (shapeInitCache.boxYOffset == 0.0) && (shapeInitCache.boxHeight == 1.0);
		}
	}
	case ArenaVoxelType::Diagonal:
		return false;
	case ArenaVoxelType::TransparentWall:
		return (facing != VoxelFacing3D::PositiveY) && (facing != VoxelFacing3D::NegativeY);
	case ArenaVoxelType::Edge:
	{
		const VoxelFacing3D edgeFacing = VoxelUtils::convertFaceTo3D(shapeInitCache.edge.facing);
		return facing == edgeFacing;
	}
	case ArenaVoxelType::Chasm:
		return facing == VoxelFacing3D::NegativeY;
	case ArenaVoxelType::Door:
		return false;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(voxelType)));
	}
}
