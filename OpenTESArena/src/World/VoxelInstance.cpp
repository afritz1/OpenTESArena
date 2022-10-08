#include <algorithm>

#include "VoxelFacing3D.h"
#include "VoxelInstance.h"

#include "components/debug/Debug.h"

void VoxelInstance::ChasmState::init(bool north, bool east, bool south, bool west)
{
	this->north = north;
	this->east = east;
	this->south = south;
	this->west = west;
}

bool VoxelInstance::ChasmState::getNorth() const
{
	return this->north;
}

bool VoxelInstance::ChasmState::getEast() const
{
	return this->east;
}

bool VoxelInstance::ChasmState::getSouth() const
{
	return this->south;
}

bool VoxelInstance::ChasmState::getWest() const
{
	return this->west;
}

bool VoxelInstance::ChasmState::faceIsVisible(VoxelFacing3D facing) const
{
	switch (facing)
	{
	case VoxelFacing3D::PositiveX:
		return this->south;
	case VoxelFacing3D::PositiveZ:
		return this->west;
	case VoxelFacing3D::NegativeX:
		return this->north;
	case VoxelFacing3D::NegativeZ:
		return this->east;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(facing)));
	}
}

bool VoxelInstance::ChasmState::faceIsVisible(VoxelFacing2D facing) const
{
	return this->faceIsVisible(VoxelUtils::convertFaceTo3D(facing));
}

int VoxelInstance::ChasmState::getFaceCount() const
{
	// Add one for floor.
	return 1 + (this->north ? 1 : 0) + (this->east ? 1 : 0) +
		(this->south ? 1 : 0) + (this->west ? 1 : 0);
}

VoxelInstance::VoxelInstance()
{
	this->x = 0;
	this->y = 0;
	this->z = 0;
	this->type = static_cast<VoxelInstance::Type>(-1);
}

void VoxelInstance::init(SNInt x, int y, WEInt z, Type type)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->type = type;
}

VoxelInstance VoxelInstance::makeChasm(SNInt x, int y, WEInt z, bool north, bool east,
	bool south, bool west)
{
	// A chasm with no walls does not need a voxel instance.
	DebugAssert(north || east || south || west);

	VoxelInstance voxelInst;
	voxelInst.init(x, y, z, Type::Chasm);
	voxelInst.chasm.init(north, east, south, west);
	return voxelInst;
}

SNInt VoxelInstance::getX() const
{
	return this->x;
}

int VoxelInstance::getY() const
{
	return this->y;
}

WEInt VoxelInstance::getZ() const
{
	return this->z;
}

VoxelInstance::Type VoxelInstance::getType() const
{
	return this->type;
}

VoxelInstance::ChasmState &VoxelInstance::getChasmState()
{
	DebugAssert(this->type == Type::Chasm);
	return this->chasm;
}

const VoxelInstance::ChasmState &VoxelInstance::getChasmState() const
{
	DebugAssert(this->type == Type::Chasm);
	return this->chasm;
}

bool VoxelInstance::hasRelevantState() const
{
	if (this->type == Type::Chasm)
	{
		const VoxelInstance::ChasmState &chasmState = this->chasm;
		return chasmState.getNorth() || chasmState.getSouth() || chasmState.getEast() || chasmState.getWest();
	}
	else
	{
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(this->type)));
	}
}
