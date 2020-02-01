#include <algorithm>
#include <array>
#include <stdexcept>

#include "VoxelData.h"
#include "VoxelDataType.h"
#include "VoxelFacing.h"
#include "../Assets/INFFile.h"
#include "../Assets/MIFFile.h"

#include "components/debug/Debug.h"

bool VoxelData::WallData::isMenu() const
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

VoxelData::WallData::MenuType VoxelData::WallData::getMenuType(int menuID, bool isCity)
{
	if (menuID != -1)
	{
		// Mappings of *MENU IDs to city menu types.
		const std::array<std::pair<int, VoxelData::WallData::MenuType>, 14> CityMenuMappings =
		{
			{
				{ 0, VoxelData::WallData::MenuType::Equipment },
				{ 1, VoxelData::WallData::MenuType::Tavern },
				{ 2, VoxelData::WallData::MenuType::MagesGuild },
				{ 3, VoxelData::WallData::MenuType::Temple },
				{ 4, VoxelData::WallData::MenuType::House },
				{ 5, VoxelData::WallData::MenuType::House },
				{ 6, VoxelData::WallData::MenuType::House },
				{ 7, VoxelData::WallData::MenuType::CityGates },
				{ 8, VoxelData::WallData::MenuType::CityGates },
				{ 9, VoxelData::WallData::MenuType::Noble },
				{ 10, VoxelData::WallData::MenuType::None },
				{ 11, VoxelData::WallData::MenuType::Palace },
				{ 12, VoxelData::WallData::MenuType::Palace },
				{ 13, VoxelData::WallData::MenuType::Palace }
			}
		};

		// Mappings of *MENU IDs to wilderness menu types.
		const std::array<std::pair<int, VoxelData::WallData::MenuType>, 10> WildMenuMappings =
		{
			{
				{ 0, VoxelData::WallData::MenuType::None },
				{ 1, VoxelData::WallData::MenuType::Crypt },
				{ 2, VoxelData::WallData::MenuType::House },
				{ 3, VoxelData::WallData::MenuType::Tavern },
				{ 4, VoxelData::WallData::MenuType::Temple },
				{ 5, VoxelData::WallData::MenuType::Tower },
				{ 6, VoxelData::WallData::MenuType::CityGates },
				{ 7, VoxelData::WallData::MenuType::CityGates },
				{ 8, VoxelData::WallData::MenuType::Dungeon },
				{ 9, VoxelData::WallData::MenuType::Dungeon }
			}
		};

		// Get the menu type associated with the *MENU ID and city boolean, or null if there
		// is no mapping (only in exceptional cases). Use a pointer since iterators are tied
		// to their std::array size.
		const VoxelData::WallData::MenuType *typePtr = [menuID, isCity, &CityMenuMappings,
			&WildMenuMappings]()
		{
			auto getPtr = [menuID](const auto &arr)
			{
				const auto iter = std::find_if(arr.begin(), arr.end(),
					[menuID](const std::pair<int, VoxelData::WallData::MenuType> &pair)
				{
					return pair.first == menuID;
				});

				return (iter != arr.end()) ? &iter->second : nullptr;
			};

			// Interpretation of *MENU ID depends on whether it's a city or wilderness.
			return isCity ? getPtr(CityMenuMappings) : getPtr(WildMenuMappings);
		}();

		// See if the array contains the associated *MENU ID.
		if (typePtr != nullptr)
		{
			return *typePtr;
		}
		else
		{
			DebugLogWarning("Unrecognized *MENU ID \"" + std::to_string(menuID) + "\".");
			return VoxelData::WallData::MenuType::None;
		}
	}
	else
	{
		// Not a *MENU block.
		return VoxelData::WallData::MenuType::None;
	}
}

bool VoxelData::WallData::menuLeadsToInterior(MenuType menuType)
{
	return (menuType == VoxelData::WallData::MenuType::Crypt) ||
		(menuType == VoxelData::WallData::MenuType::Dungeon) ||
		(menuType == VoxelData::WallData::MenuType::Equipment) ||
		(menuType == VoxelData::WallData::MenuType::House) ||
		(menuType == VoxelData::WallData::MenuType::MagesGuild) ||
		(menuType == VoxelData::WallData::MenuType::Noble) ||
		(menuType == VoxelData::WallData::MenuType::Palace) ||
		(menuType == VoxelData::WallData::MenuType::Tavern) ||
		(menuType == VoxelData::WallData::MenuType::Temple) ||
		(menuType == VoxelData::WallData::MenuType::Tower);
}

bool VoxelData::WallData::menuHasDisplayName(MenuType menuType)
{
	return (menuType == VoxelData::WallData::MenuType::Equipment) ||
		(menuType == VoxelData::WallData::MenuType::MagesGuild) ||
		(menuType == VoxelData::WallData::MenuType::Tavern) ||
		(menuType == VoxelData::WallData::MenuType::Temple);
}

const double VoxelData::ChasmData::WET_LAVA_DEPTH = static_cast<double>(
	INFFile::CeilingData::DEFAULT_HEIGHT) / MIFFile::ARENA_UNITS;

bool VoxelData::ChasmData::matches(const ChasmData &other) const
{
	return (this->id == other.id) && (this->north == other.north) && (this->east == other.east) &&
		(this->south == other.south) && (this->west == other.west) && (this->type == other.type);
}

bool VoxelData::ChasmData::faceIsVisible(VoxelFacing facing) const
{
	if (facing == VoxelFacing::PositiveX)
	{
		return this->north;
	}
	else if (facing == VoxelFacing::PositiveZ)
	{
		return this->east;
	}
	else if (facing == VoxelFacing::NegativeX)
	{
		return this->south;
	}
	else
	{
		return this->west;
	}
}

int VoxelData::ChasmData::getFaceCount() const
{
	// Assume chasms have floors.
	return 1 + (this->north ? 1 : 0) + (this->east ? 1 : 0) +
		(this->south ? 1 : 0) + (this->west ? 1 : 0);
}

int VoxelData::DoorData::getOpenSoundIndex() const
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

VoxelData::DoorData::CloseSoundData VoxelData::DoorData::getCloseSoundData() const
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
		DebugUnhandledReturnMsg(VoxelData::DoorData::CloseSoundData,
			std::to_string(static_cast<int>(this->type)));
	}
}

const int VoxelData::TOTAL_IDS = 64;

VoxelData::VoxelData()
{
	// Default to empty.
	this->dataType = VoxelDataType::None;
}

VoxelData VoxelData::makeWall(int sideID, int floorID, int ceilingID,
	const int *menuID, WallData::Type type)
{
	if (sideID >= VoxelData::TOTAL_IDS)
	{
		DebugLogWarning("Wall side ID \"" + std::to_string(sideID) + "\" out of range.");
	}

	if (floorID >= VoxelData::TOTAL_IDS)
	{
		DebugLogWarning("Wall floor ID \"" + std::to_string(floorID) + "\" out of range.");
	}

	if (ceilingID >= VoxelData::TOTAL_IDS)
	{
		DebugLogWarning("Wall ceiling ID \"" + std::to_string(ceilingID) + "\" out of range.");
	}

	VoxelData data;
	data.dataType = VoxelDataType::Wall;

	VoxelData::WallData &wall = data.wall;
	wall.sideID = sideID % VoxelData::TOTAL_IDS;
	wall.floorID = floorID % VoxelData::TOTAL_IDS;
	wall.ceilingID = ceilingID % VoxelData::TOTAL_IDS;

	// If the menu ID parameter is given, use it.
	if (menuID != nullptr)
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

VoxelData VoxelData::makeFloor(int id)
{
	if (id >= VoxelData::TOTAL_IDS)
	{
		DebugLogWarning("Floor ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelData data;
	data.dataType = VoxelDataType::Floor;

	VoxelData::FloorData &floor = data.floor;
	floor.id = id % VoxelData::TOTAL_IDS;

	return data;
}

VoxelData VoxelData::makeCeiling(int id)
{
	if (id >= VoxelData::TOTAL_IDS)
	{
		DebugLogWarning("Ceiling ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelData data;
	data.dataType = VoxelDataType::Ceiling;

	VoxelData::CeilingData &ceiling = data.ceiling;
	ceiling.id = id % VoxelData::TOTAL_IDS;

	return data;
}

VoxelData VoxelData::makeRaised(int sideID, int floorID, int ceilingID, double yOffset,
	double ySize, double vTop, double vBottom)
{
	if (sideID >= VoxelData::TOTAL_IDS)
	{
		DebugLogWarning("Raised side ID \"" + std::to_string(sideID) + "\" out of range.");
	}

	if (floorID >= VoxelData::TOTAL_IDS)
	{
		DebugLogWarning("Raised floor ID \"" + std::to_string(floorID) + "\" out of range.");
	}

	if (ceilingID >= VoxelData::TOTAL_IDS)
	{
		DebugLogWarning("Raised ceiling ID \"" + std::to_string(ceilingID) + "\" out of range.");
	}

	VoxelData data;
	data.dataType = VoxelDataType::Raised;

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
		DebugLogWarning("Diagonal ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelData data;
	data.dataType = VoxelDataType::Diagonal;

	VoxelData::DiagonalData &diagonal = data.diagonal;
	diagonal.id = id % VoxelData::TOTAL_IDS;
	diagonal.type1 = type1;

	return data;
}

VoxelData VoxelData::makeTransparentWall(int id, bool collider)
{
	if (id >= VoxelData::TOTAL_IDS)
	{
		DebugLogWarning("Transparent wall ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelData data;
	data.dataType = VoxelDataType::TransparentWall;

	VoxelData::TransparentWallData &transparentWall = data.transparentWall;
	transparentWall.id = id % VoxelData::TOTAL_IDS;
	transparentWall.collider = collider;

	return data;
}

VoxelData VoxelData::makeEdge(int id, double yOffset, bool collider, bool flipped, VoxelFacing facing)
{
	if (id >= VoxelData::TOTAL_IDS)
	{
		DebugLogWarning("Edge ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelData data;
	data.dataType = VoxelDataType::Edge;

	VoxelData::EdgeData &edge = data.edge;
	edge.id = id % VoxelData::TOTAL_IDS;
	edge.yOffset = yOffset;
	edge.collider = collider;
	edge.flipped = flipped;
	edge.facing = facing;

	return data;
}

VoxelData VoxelData::makeChasm(int id, bool north, bool east, bool south, bool west,
	ChasmData::Type type)
{
	if (id >= VoxelData::TOTAL_IDS)
	{
		DebugLogWarning("Chasm ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelData data;
	data.dataType = VoxelDataType::Chasm;

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
		DebugLogWarning("Door ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelData data;
	data.dataType = VoxelDataType::Door;

	VoxelData::DoorData &door = data.door;
	door.id = id % VoxelData::TOTAL_IDS;
	door.type = type;

	return data;
}

Double3 VoxelData::getNormal(VoxelFacing facing)
{
	// Decide what the normal is, based on the facing.
	if (facing == VoxelFacing::PositiveX)
	{
		return Double3::UnitX;
	}
	else if (facing == VoxelFacing::NegativeX)
	{
		return -Double3::UnitX;
	}
	else if (facing == VoxelFacing::PositiveZ)
	{
		return Double3::UnitZ;
	}
	else
	{
		return -Double3::UnitZ;
	}
}
