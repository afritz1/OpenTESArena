#include "ArenaMeshUtils.h"
#include "MeshLibrary.h"
#include "../Math/Constants.h"
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

void ArenaShapeInitCache::initChasmBoxValues(bool isDryChasm)
{
	// Offset below the chasm floor so the collider isn't infinitely thin.
	// @todo: this doesn't seem right for wet chasms
	this->boxWidth = 1.0;
	this->boxHeight = 0.10;
	if (!isDryChasm)
	{
		this->boxHeight += 1.0 - ArenaChasmUtils::DEFAULT_HEIGHT;
	}

	this->boxDepth = 1.0;
	this->boxYOffset = -0.10;
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

void ArenaMeshUtils::writeChasmWallRendererIndexBuffers(ArenaChasmWallIndexBuffer *outNorthIndices, ArenaChasmWallIndexBuffer *outEastIndices,
	ArenaChasmWallIndexBuffer *outSouthIndices, ArenaChasmWallIndexBuffer *outWestIndices)
{
	if (outNorthIndices != nullptr) // X=0
	{
		(*outNorthIndices)[0] = 4;
		(*outNorthIndices)[1] = 5;
		(*outNorthIndices)[2] = 6;
		(*outNorthIndices)[3] = 6;
		(*outNorthIndices)[4] = 7;
		(*outNorthIndices)[5] = 4;
	}

	if (outEastIndices != nullptr) // Z=0
	{
		(*outEastIndices)[0] = 12;
		(*outEastIndices)[1] = 13;
		(*outEastIndices)[2] = 14;
		(*outEastIndices)[3] = 14;
		(*outEastIndices)[4] = 15;
		(*outEastIndices)[5] = 12;
	}

	if (outSouthIndices != nullptr) // X=1
	{
		(*outSouthIndices)[0] = 8;
		(*outSouthIndices)[1] = 9;
		(*outSouthIndices)[2] = 10;
		(*outSouthIndices)[3] = 10;
		(*outSouthIndices)[4] = 11;
		(*outSouthIndices)[5] = 8;
	}

	if (outWestIndices != nullptr) // Z=1
	{
		(*outWestIndices)[0] = 16;
		(*outWestIndices)[1] = 17;
		(*outWestIndices)[2] = 18;
		(*outWestIndices)[3] = 18;
		(*outWestIndices)[4] = 19;
		(*outWestIndices)[5] = 16;
	}
}
