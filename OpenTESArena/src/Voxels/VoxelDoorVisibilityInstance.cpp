#include "VoxelDoorVisibilityInstance.h"

#include "components/debug/Debug.h"

namespace
{
	constexpr VoxelFacing2D INVALID_FACING = static_cast<VoxelFacing2D>(-1);
}

VoxelDoorVisibilityInstance::VoxelDoorVisibilityInstance()
{
	this->clear();
}

void VoxelDoorVisibilityInstance::init(SNInt x, int y, WEInt z)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->clearVisibleFaces();
}

void VoxelDoorVisibilityInstance::clearVisibleFaces()
{
	this->visibleFaces[0] = INVALID_FACING;
	this->visibleFaces[1] = INVALID_FACING;
	this->visibleFaceCount = 0;
}

void VoxelDoorVisibilityInstance::clear()
{
	this->x = 0;
	this->y = 0;
	this->z = 0;
	this->clearVisibleFaces();
}

void VoxelDoorVisibilityInstance::update(bool isCameraNorthInclusive, bool isCameraEastInclusive, bool isNorthValid, bool isEastValid,
	bool isSouthValid, bool isWestValid)
{
	this->clearVisibleFaces();

	auto tryAddVisibleFace = [this](VoxelFacing2D facing, bool isAdjacentVoxelValid)
	{
		if (isAdjacentVoxelValid)
		{
			DebugAssertIndex(this->visibleFaces, this->visibleFaceCount);
			this->visibleFaces[this->visibleFaceCount] = facing;
			this->visibleFaceCount++;
		}
	};

	if (isCameraNorthInclusive)
	{
		tryAddVisibleFace(VoxelFacing2D::NegativeX, isNorthValid);
	}
	else
	{
		tryAddVisibleFace(VoxelFacing2D::PositiveX, isSouthValid);
	}

	if (isCameraEastInclusive)
	{
		tryAddVisibleFace(VoxelFacing2D::NegativeZ, isEastValid);
	}
	else
	{
		tryAddVisibleFace(VoxelFacing2D::PositiveZ, isWestValid);
	}
}
