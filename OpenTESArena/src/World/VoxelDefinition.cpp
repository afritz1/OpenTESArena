#include <algorithm>
#include <array>
#include <stdexcept>

#include "ArenaVoxelUtils.h"
#include "VoxelDefinition.h"
#include "VoxelDataType.h"
#include "VoxelFacing2D.h"
#include "WorldType.h"
#include "../Assets/INFFile.h"
#include "../Assets/MIFUtils.h"

#include "components/debug/Debug.h"

void VoxelDefinition::WallData::init(int sideID, int floorID, int ceilingID, int menuID, Type type)
{
	this->sideID = sideID;
	this->floorID = floorID;
	this->ceilingID = ceilingID;
	this->menuID = menuID;
	this->type = type;
}

bool VoxelDefinition::WallData::isMenu() const
{
	if (this->type == WallData::Type::Menu)
	{
		DebugAssert(this->menuID != -1);
		return true;
	}
	else
	{
		return false;
	}
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
	this->dataType = VoxelDataType::None;
}

VoxelDefinition VoxelDefinition::makeWall(int sideID, int floorID, int ceilingID,
	const std::optional<int> &menuID, WallData::Type type)
{
	DebugAssert((type == WallData::Type::Menu) == menuID.has_value());

	// @todo: move these checks and modulos to when the Arena IDs are originally obtained from level data.
	// - VoxelDefinition should not care about Arena-related values.
	if (sideID >= ArenaVoxelUtils::TOTAL_VOXEL_IDS)
	{
		DebugLogWarning("Wall side ID \"" + std::to_string(sideID) + "\" out of range.");
	}

	if (floorID >= ArenaVoxelUtils::TOTAL_VOXEL_IDS)
	{
		DebugLogWarning("Wall floor ID \"" + std::to_string(floorID) + "\" out of range.");
	}

	if (ceilingID >= ArenaVoxelUtils::TOTAL_VOXEL_IDS)
	{
		DebugLogWarning("Wall ceiling ID \"" + std::to_string(ceilingID) + "\" out of range.");
	}

	VoxelDefinition voxelDef;
	voxelDef.dataType = VoxelDataType::Wall;
	voxelDef.wall.init(sideID % ArenaVoxelUtils::TOTAL_VOXEL_IDS, floorID % ArenaVoxelUtils::TOTAL_VOXEL_IDS,
		ceilingID % ArenaVoxelUtils::TOTAL_VOXEL_IDS, menuID.has_value() ? *menuID : -1, type);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeFloor(int id)
{
	if (id >= ArenaVoxelUtils::TOTAL_VOXEL_IDS)
	{
		DebugLogWarning("Floor ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelDefinition voxelDef;
	voxelDef.dataType = VoxelDataType::Floor;
	voxelDef.floor.init(id % ArenaVoxelUtils::TOTAL_VOXEL_IDS);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeCeiling(int id)
{
	if (id >= ArenaVoxelUtils::TOTAL_VOXEL_IDS)
	{
		DebugLogWarning("Ceiling ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelDefinition voxelDef;
	voxelDef.dataType = VoxelDataType::Ceiling;
	voxelDef.ceiling.init(id % ArenaVoxelUtils::TOTAL_VOXEL_IDS);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeRaised(int sideID, int floorID, int ceilingID, double yOffset,
	double ySize, double vTop, double vBottom)
{
	if (sideID >= ArenaVoxelUtils::TOTAL_VOXEL_IDS)
	{
		DebugLogWarning("Raised side ID \"" + std::to_string(sideID) + "\" out of range.");
	}

	if (floorID >= ArenaVoxelUtils::TOTAL_VOXEL_IDS)
	{
		DebugLogWarning("Raised floor ID \"" + std::to_string(floorID) + "\" out of range.");
	}

	if (ceilingID >= ArenaVoxelUtils::TOTAL_VOXEL_IDS)
	{
		DebugLogWarning("Raised ceiling ID \"" + std::to_string(ceilingID) + "\" out of range.");
	}

	VoxelDefinition voxelDef;
	voxelDef.dataType = VoxelDataType::Raised;
	voxelDef.raised.init(sideID % ArenaVoxelUtils::TOTAL_VOXEL_IDS, floorID % ArenaVoxelUtils::TOTAL_VOXEL_IDS,
		ceilingID % ArenaVoxelUtils::TOTAL_VOXEL_IDS, yOffset, ySize, vTop, vBottom);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeDiagonal(int id, bool type1)
{
	if (id >= ArenaVoxelUtils::TOTAL_VOXEL_IDS)
	{
		DebugLogWarning("Diagonal ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelDefinition voxelDef;
	voxelDef.dataType = VoxelDataType::Diagonal;
	voxelDef.diagonal.init(id % ArenaVoxelUtils::TOTAL_VOXEL_IDS, type1);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeTransparentWall(int id, bool collider)
{
	if (id >= ArenaVoxelUtils::TOTAL_VOXEL_IDS)
	{
		DebugLogWarning("Transparent wall ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelDefinition voxelDef;
	voxelDef.dataType = VoxelDataType::TransparentWall;
	voxelDef.transparentWall.init(id % ArenaVoxelUtils::TOTAL_VOXEL_IDS, collider);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeEdge(int id, double yOffset, bool collider,
	bool flipped, VoxelFacing2D facing)
{
	if (id >= ArenaVoxelUtils::TOTAL_VOXEL_IDS)
	{
		DebugLogWarning("Edge ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelDefinition voxelDef;
	voxelDef.dataType = VoxelDataType::Edge;
	voxelDef.edge.init(id % ArenaVoxelUtils::TOTAL_VOXEL_IDS, yOffset, collider, flipped, facing);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeChasm(int id, ChasmData::Type type)
{
	if (id >= ArenaVoxelUtils::TOTAL_VOXEL_IDS)
	{
		DebugLogWarning("Chasm ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelDefinition voxelDef;
	voxelDef.dataType = VoxelDataType::Chasm;
	voxelDef.chasm.init(id % ArenaVoxelUtils::TOTAL_VOXEL_IDS, type);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeDoor(int id, DoorData::Type type)
{
	if (id >= ArenaVoxelUtils::TOTAL_VOXEL_IDS)
	{
		DebugLogWarning("Door ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelDefinition voxelDef;
	voxelDef.dataType = VoxelDataType::Door;
	voxelDef.door.init(id % ArenaVoxelUtils::TOTAL_VOXEL_IDS, type);
	return voxelDef;
}

bool VoxelDefinition::allowsChasmFace() const
{
	return (this->dataType != VoxelDataType::None) && (this->dataType != VoxelDataType::Chasm);
}
