#include "MapType.h"
#include "VoxelDefinition.h"
#include "VoxelFacing2D.h"
#include "VoxelType.h"

#include "components/debug/Debug.h"

void VoxelDefinition::WallData::init(int sideID, int floorID, int ceilingID)
{
	this->sideID = sideID;
	this->floorID = floorID;
	this->ceilingID = ceilingID;
}

void VoxelDefinition::FloorData::init(int id)
{
	this->id = id;
}

void VoxelDefinition::CeilingData::init(int id)
{
	this->id = id;
}

void VoxelDefinition::RaisedData::init(int sideID, int floorID, int ceilingID, double yOffset, double ySize,
	double vTop, double vBottom)
{
	this->sideID = sideID;
	this->floorID = floorID;
	this->ceilingID = ceilingID;
	this->yOffset = yOffset;
	this->ySize = ySize;
	this->vTop = vTop;
	this->vBottom = vBottom;
}

void VoxelDefinition::DiagonalData::init(int id, bool type1)
{
	this->id = id;
	this->type1 = type1;
}

void VoxelDefinition::TransparentWallData::init(int id, bool collider)
{
	this->id = id;
	this->collider = collider;
}

void VoxelDefinition::EdgeData::init(int id, double yOffset, bool collider, bool flipped, VoxelFacing2D facing)
{
	this->id = id;
	this->yOffset = yOffset;
	this->collider = collider;
	this->flipped = flipped;
	this->facing = facing;
}

void VoxelDefinition::ChasmData::init(int id, Type type)
{
	this->id = id;
	this->type = type;
}

bool VoxelDefinition::ChasmData::matches(const ChasmData &other) const
{
	return (this->id == other.id) && (this->type == other.type);
}

void VoxelDefinition::DoorData::init(int id, Type type)
{
	this->id = id;
	this->type = type;
}

int VoxelDefinition::DoorData::getOpenSoundIndex() const
{
	if (this->type == DoorData::Type::Swinging)
	{
		return 6;
	}
	else if (this->type == DoorData::Type::Sliding)
	{
		return 14;
	}
	else if (this->type == DoorData::Type::Raising)
	{
		return 15;
	}
	else
	{
		DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(this->type)));
	}
}

VoxelDefinition::DoorData::CloseSoundData VoxelDefinition::DoorData::getCloseSoundData() const
{
	if (this->type == DoorData::Type::Swinging)
	{
		return { 5, DoorData::CloseSoundType::OnClosed };
	}
	else if (this->type == DoorData::Type::Sliding)
	{
		return { 14, DoorData::CloseSoundType::OnClosing };
	}
	else if (this->type == DoorData::Type::Raising)
	{
		return { 15, DoorData::CloseSoundType::OnClosing };
	}
	else
	{
		DebugUnhandledReturnMsg(VoxelDefinition::DoorData::CloseSoundData,
			std::to_string(static_cast<int>(this->type)));
	}
}

VoxelDefinition::VoxelDefinition()
{
	// Default to empty.
	this->type = VoxelType::None;
}

VoxelDefinition VoxelDefinition::makeWall(int sideID, int floorID, int ceilingID)
{
	VoxelDefinition voxelDef;
	voxelDef.type = VoxelType::Wall;
	voxelDef.wall.init(sideID, floorID, ceilingID);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeFloor(int id)
{
	VoxelDefinition voxelDef;
	voxelDef.type = VoxelType::Floor;
	voxelDef.floor.init(id);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeCeiling(int id)
{
	VoxelDefinition voxelDef;
	voxelDef.type = VoxelType::Ceiling;
	voxelDef.ceiling.init(id);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeRaised(int sideID, int floorID, int ceilingID, double yOffset,
	double ySize, double vTop, double vBottom)
{
	VoxelDefinition voxelDef;
	voxelDef.type = VoxelType::Raised;
	voxelDef.raised.init(sideID, floorID, ceilingID, yOffset, ySize, vTop, vBottom);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeDiagonal(int id, bool type1)
{
	VoxelDefinition voxelDef;
	voxelDef.type = VoxelType::Diagonal;
	voxelDef.diagonal.init(id, type1);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeTransparentWall(int id, bool collider)
{
	VoxelDefinition voxelDef;
	voxelDef.type = VoxelType::TransparentWall;
	voxelDef.transparentWall.init(id, collider);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeEdge(int id, double yOffset, bool collider,
	bool flipped, VoxelFacing2D facing)
{
	VoxelDefinition voxelDef;
	voxelDef.type = VoxelType::Edge;
	voxelDef.edge.init(id, yOffset, collider, flipped, facing);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeChasm(int id, ChasmData::Type type)
{
	VoxelDefinition voxelDef;
	voxelDef.type = VoxelType::Chasm;
	voxelDef.chasm.init(id, type);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeDoor(int id, DoorData::Type type)
{
	VoxelDefinition voxelDef;
	voxelDef.type = VoxelType::Door;
	voxelDef.door.init(id, type);
	return voxelDef;
}

bool VoxelDefinition::allowsChasmFace() const
{
	return (this->type != VoxelType::None) && (this->type != VoxelType::Chasm);
}
