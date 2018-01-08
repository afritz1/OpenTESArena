#include <stdexcept>

#include "VoxelData.h"
#include "VoxelDataType.h"
#include "VoxelType.h"
#include "../Utilities/Debug.h"

const int VoxelData::WallData::NO_MENU = -1;

bool VoxelData::ChasmData::faceIsVisible(VoxelData::Facing facing) const
{
	if (facing == VoxelData::Facing::PositiveX)
	{
		return this->north;
	}
	else if (facing == VoxelData::Facing::PositiveZ)
	{
		return this->east;
	}
	else if (facing == VoxelData::Facing::NegativeX)
	{
		return this->south;
	}
	else
	{
		return this->west;
	}
}

const int VoxelData::TOTAL_IDS = 64;

VoxelData::VoxelData()
{
	// Default to empty.
	this->dataType = VoxelDataType::None;
	this->type = VoxelType::Empty;
}

VoxelData::~VoxelData()
{

}

VoxelData VoxelData::makeWall(int sideID, int floorID, int ceilingID, VoxelType type)
{
	if (sideID >= VoxelData::TOTAL_IDS)
	{
		DebugWarning("Wall side ID \"" + std::to_string(sideID) + "\" out of range.");
	}

	if (floorID >= VoxelData::TOTAL_IDS)
	{
		DebugWarning("Wall floor ID \"" + std::to_string(floorID) + "\" out of range.");
	}

	if (ceilingID >= VoxelData::TOTAL_IDS)
	{
		DebugWarning("Wall ceiling ID \"" + std::to_string(ceilingID) + "\" out of range.");
	}

	VoxelData data;
	data.dataType = VoxelDataType::Wall;
	data.type = type;

	VoxelData::WallData &wall = data.wall;
	wall.sideID = sideID % VoxelData::TOTAL_IDS;
	wall.floorID = floorID % VoxelData::TOTAL_IDS;
	wall.ceilingID = ceilingID % VoxelData::TOTAL_IDS;
	wall.menuID = VoxelData::WallData::NO_MENU; // Assigned outside this method.

	return data;
}

VoxelData VoxelData::makeFloor(int id)
{
	if (id >= VoxelData::TOTAL_IDS)
	{
		DebugWarning("Floor ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelData data;
	data.dataType = VoxelDataType::Floor;
	data.type = VoxelType::Solid;

	VoxelData::FloorData &floor = data.floor;
	floor.id = id % VoxelData::TOTAL_IDS;

	return data;
}

VoxelData VoxelData::makeCeiling(int id)
{
	if (id >= VoxelData::TOTAL_IDS)
	{
		DebugWarning("Ceiling ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelData data;
	data.dataType = VoxelDataType::Ceiling;
	data.type = VoxelType::Solid;

	VoxelData::CeilingData &ceiling = data.ceiling;
	ceiling.id = id % VoxelData::TOTAL_IDS;

	return data;
}

VoxelData VoxelData::makeRaised(int sideID, int floorID, int ceilingID, double yOffset,
	double ySize, double vTop, double vBottom)
{
	if (sideID >= VoxelData::TOTAL_IDS)
	{
		DebugWarning("Raised side ID \"" + std::to_string(sideID) + "\" out of range.");
	}

	if (floorID >= VoxelData::TOTAL_IDS)
	{
		DebugWarning("Raised floor ID \"" + std::to_string(floorID) + "\" out of range.");
	}

	if (ceilingID >= VoxelData::TOTAL_IDS)
	{
		DebugWarning("Raised ceiling ID \"" + std::to_string(ceilingID) + "\" out of range.");
	}

	VoxelData data;
	data.dataType = VoxelDataType::Raised;
	data.type = VoxelType::Raised;

	VoxelData::RaisedData &raised = data.raised;
	raised.sideID = sideID % VoxelData::TOTAL_IDS;
	raised.floorID = floorID % VoxelData::TOTAL_IDS;
	raised.ceilingID = ceilingID % VoxelData::TOTAL_IDS;
	raised.yOffset = yOffset;
	raised.ySize = ySize;
	raised.vTop = vTop;
	raised.vBottom = vBottom;

	return data;
}

VoxelData VoxelData::makeDiagonal(int id, bool type1)
{
	if (id >= VoxelData::TOTAL_IDS)
	{
		DebugWarning("Diagonal ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelData data;
	data.dataType = VoxelDataType::Diagonal;
	data.type = VoxelType::Diagonal;

	VoxelData::DiagonalData &diagonal = data.diagonal;
	diagonal.id = id % VoxelData::TOTAL_IDS;
	diagonal.type1 = type1;

	return data;
}

VoxelData VoxelData::makeTransparentWall(int id)
{
	if (id >= VoxelData::TOTAL_IDS)
	{
		DebugWarning("Transparent wall ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelData data;
	data.dataType = VoxelDataType::TransparentWall;
	data.type = VoxelType::TransparentWall;

	VoxelData::TransparentWallData &transparentWall = data.transparentWall;
	transparentWall.id = id % VoxelData::TOTAL_IDS;

	return data;
}

VoxelData VoxelData::makeEdge(int id, Facing facing)
{
	if (id >= VoxelData::TOTAL_IDS)
	{
		DebugWarning("Edge ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelData data;
	data.dataType = VoxelDataType::Edge;
	data.type = VoxelType::Edge;

	VoxelData::EdgeData &edge = data.edge;
	edge.id = id % VoxelData::TOTAL_IDS;
	edge.facing = facing;

	return data;
}

VoxelData VoxelData::makeChasm(int id, bool north, bool east, bool south, bool west,
	ChasmData::Type type)
{
	if (id >= VoxelData::TOTAL_IDS)
	{
		DebugWarning("Chasm ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelData data;
	data.dataType = VoxelDataType::Chasm;
	data.type = [type]()
	{
		if (type == ChasmData::Type::Dry)
		{
			return VoxelType::DryChasm;
		}
		else if (type == ChasmData::Type::Wet)
		{
			return VoxelType::WetChasm;
		}
		else if (type == ChasmData::Type::Lava)
		{
			return VoxelType::LavaChasm;
		}
		else
		{
			throw std::runtime_error("Bad chasm type.");
		}
	}();

	VoxelData::ChasmData &chasm = data.chasm;
	chasm.id = id % VoxelData::TOTAL_IDS;
	chasm.north = north;
	chasm.east = east;
	chasm.south = south;
	chasm.west = west;
	chasm.type = type;

	return data;
}

VoxelData VoxelData::makeDoor(int id, DoorData::Type type)
{
	if (id >= VoxelData::TOTAL_IDS)
	{
		DebugWarning("Door ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelData data;
	data.dataType = VoxelDataType::Door;
	data.type = VoxelType::Door;

	VoxelData::DoorData &door = data.door;
	door.id = id % VoxelData::TOTAL_IDS;
	door.type = type;

	return data;
}
