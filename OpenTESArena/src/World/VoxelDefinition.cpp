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

bool VoxelDefinition::ChasmData::matches(const ChasmData &other) const
{
	return (this->id == other.id) && (this->type == other.type);
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

	VoxelDefinition data;
	data.dataType = VoxelDataType::Wall;

	VoxelDefinition::WallData &wall = data.wall;
	wall.sideID = sideID % ArenaVoxelUtils::TOTAL_VOXEL_IDS;
	wall.floorID = floorID % ArenaVoxelUtils::TOTAL_VOXEL_IDS;
	wall.ceilingID = ceilingID % ArenaVoxelUtils::TOTAL_VOXEL_IDS;

	// If the menu ID parameter is given, use it.
	if (menuID.has_value())
	{
		DebugAssert(type == WallData::Type::Menu);
		wall.menuID = *menuID;
	}
	else
	{
		DebugAssert(type != WallData::Type::Menu);
		wall.menuID = -1; // Unused.
	}

	wall.type = type;

	return data;
}

VoxelDefinition VoxelDefinition::makeFloor(int id)
{
	if (id >= ArenaVoxelUtils::TOTAL_VOXEL_IDS)
	{
		DebugLogWarning("Floor ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelDefinition data;
	data.dataType = VoxelDataType::Floor;

	VoxelDefinition::FloorData &floor = data.floor;
	floor.id = id % ArenaVoxelUtils::TOTAL_VOXEL_IDS;

	return data;
}

VoxelDefinition VoxelDefinition::makeCeiling(int id)
{
	if (id >= ArenaVoxelUtils::TOTAL_VOXEL_IDS)
	{
		DebugLogWarning("Ceiling ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelDefinition data;
	data.dataType = VoxelDataType::Ceiling;

	VoxelDefinition::CeilingData &ceiling = data.ceiling;
	ceiling.id = id % ArenaVoxelUtils::TOTAL_VOXEL_IDS;

	return data;
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

	VoxelDefinition data;
	data.dataType = VoxelDataType::Raised;

	VoxelDefinition::RaisedData &raised = data.raised;
	raised.sideID = sideID % ArenaVoxelUtils::TOTAL_VOXEL_IDS;
	raised.floorID = floorID % ArenaVoxelUtils::TOTAL_VOXEL_IDS;
	raised.ceilingID = ceilingID % ArenaVoxelUtils::TOTAL_VOXEL_IDS;
	raised.yOffset = yOffset;
	raised.ySize = ySize;
	raised.vTop = vTop;
	raised.vBottom = vBottom;

	return data;
}

VoxelDefinition VoxelDefinition::makeDiagonal(int id, bool type1)
{
	if (id >= ArenaVoxelUtils::TOTAL_VOXEL_IDS)
	{
		DebugLogWarning("Diagonal ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelDefinition data;
	data.dataType = VoxelDataType::Diagonal;

	VoxelDefinition::DiagonalData &diagonal = data.diagonal;
	diagonal.id = id % ArenaVoxelUtils::TOTAL_VOXEL_IDS;
	diagonal.type1 = type1;

	return data;
}

VoxelDefinition VoxelDefinition::makeTransparentWall(int id, bool collider)
{
	if (id >= ArenaVoxelUtils::TOTAL_VOXEL_IDS)
	{
		DebugLogWarning("Transparent wall ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelDefinition data;
	data.dataType = VoxelDataType::TransparentWall;

	VoxelDefinition::TransparentWallData &transparentWall = data.transparentWall;
	transparentWall.id = id % ArenaVoxelUtils::TOTAL_VOXEL_IDS;
	transparentWall.collider = collider;

	return data;
}

VoxelDefinition VoxelDefinition::makeEdge(int id, double yOffset, bool collider,
	bool flipped, VoxelFacing2D facing)
{
	if (id >= ArenaVoxelUtils::TOTAL_VOXEL_IDS)
	{
		DebugLogWarning("Edge ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelDefinition data;
	data.dataType = VoxelDataType::Edge;

	VoxelDefinition::EdgeData &edge = data.edge;
	edge.id = id % ArenaVoxelUtils::TOTAL_VOXEL_IDS;
	edge.yOffset = yOffset;
	edge.collider = collider;
	edge.flipped = flipped;
	edge.facing = facing;

	return data;
}

VoxelDefinition VoxelDefinition::makeChasm(int id, ChasmData::Type type)
{
	if (id >= ArenaVoxelUtils::TOTAL_VOXEL_IDS)
	{
		DebugLogWarning("Chasm ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelDefinition data;
	data.dataType = VoxelDataType::Chasm;

	VoxelDefinition::ChasmData &chasm = data.chasm;
	chasm.id = id % ArenaVoxelUtils::TOTAL_VOXEL_IDS;
	chasm.type = type;

	return data;
}

VoxelDefinition VoxelDefinition::makeDoor(int id, DoorData::Type type)
{
	if (id >= ArenaVoxelUtils::TOTAL_VOXEL_IDS)
	{
		DebugLogWarning("Door ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelDefinition data;
	data.dataType = VoxelDataType::Door;

	VoxelDefinition::DoorData &door = data.door;
	door.id = id % ArenaVoxelUtils::TOTAL_VOXEL_IDS;
	door.type = type;

	return data;
}

bool VoxelDefinition::allowsChasmFace() const
{
	return (this->dataType != VoxelDataType::None) && (this->dataType != VoxelDataType::Chasm);
}
