#include <algorithm>
#include <array>
#include <stdexcept>

#include "VoxelDefinition.h"
#include "VoxelDataType.h"
#include "VoxelFacing.h"
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

VoxelDefinition::WallData::MenuType VoxelDefinition::WallData::getMenuType(int menuID, bool isCity)
{
	if (menuID != -1)
	{
		// Mappings of *MENU IDs to city menu types.
		const std::array<std::pair<int, VoxelDefinition::WallData::MenuType>, 14> CityMenuMappings =
		{
			{
				{ 0, VoxelDefinition::WallData::MenuType::Equipment },
				{ 1, VoxelDefinition::WallData::MenuType::Tavern },
				{ 2, VoxelDefinition::WallData::MenuType::MagesGuild },
				{ 3, VoxelDefinition::WallData::MenuType::Temple },
				{ 4, VoxelDefinition::WallData::MenuType::House },
				{ 5, VoxelDefinition::WallData::MenuType::House },
				{ 6, VoxelDefinition::WallData::MenuType::House },
				{ 7, VoxelDefinition::WallData::MenuType::CityGates },
				{ 8, VoxelDefinition::WallData::MenuType::CityGates },
				{ 9, VoxelDefinition::WallData::MenuType::Noble },
				{ 10, VoxelDefinition::WallData::MenuType::None },
				{ 11, VoxelDefinition::WallData::MenuType::Palace },
				{ 12, VoxelDefinition::WallData::MenuType::Palace },
				{ 13, VoxelDefinition::WallData::MenuType::Palace }
			}
		};

		// Mappings of *MENU IDs to wilderness menu types.
		const std::array<std::pair<int, VoxelDefinition::WallData::MenuType>, 10> WildMenuMappings =
		{
			{
				{ 0, VoxelDefinition::WallData::MenuType::None },
				{ 1, VoxelDefinition::WallData::MenuType::Crypt },
				{ 2, VoxelDefinition::WallData::MenuType::House },
				{ 3, VoxelDefinition::WallData::MenuType::Tavern },
				{ 4, VoxelDefinition::WallData::MenuType::Temple },
				{ 5, VoxelDefinition::WallData::MenuType::Tower },
				{ 6, VoxelDefinition::WallData::MenuType::CityGates },
				{ 7, VoxelDefinition::WallData::MenuType::CityGates },
				{ 8, VoxelDefinition::WallData::MenuType::Dungeon },
				{ 9, VoxelDefinition::WallData::MenuType::Dungeon }
			}
		};

		// Get the menu type associated with the *MENU ID and city boolean, or null if there
		// is no mapping (only in exceptional cases). Use a pointer since iterators are tied
		// to their std::array size.
		const VoxelDefinition::WallData::MenuType *typePtr = [menuID, isCity, &CityMenuMappings,
			&WildMenuMappings]()
		{
			auto getPtr = [menuID](const auto &arr)
			{
				const auto iter = std::find_if(arr.begin(), arr.end(),
					[menuID](const std::pair<int, VoxelDefinition::WallData::MenuType> &pair)
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
			return VoxelDefinition::WallData::MenuType::None;
		}
	}
	else
	{
		// Not a *MENU block.
		return VoxelDefinition::WallData::MenuType::None;
	}
}

bool VoxelDefinition::WallData::menuLeadsToInterior(MenuType menuType)
{
	return (menuType == VoxelDefinition::WallData::MenuType::Crypt) ||
		(menuType == VoxelDefinition::WallData::MenuType::Dungeon) ||
		(menuType == VoxelDefinition::WallData::MenuType::Equipment) ||
		(menuType == VoxelDefinition::WallData::MenuType::House) ||
		(menuType == VoxelDefinition::WallData::MenuType::MagesGuild) ||
		(menuType == VoxelDefinition::WallData::MenuType::Noble) ||
		(menuType == VoxelDefinition::WallData::MenuType::Palace) ||
		(menuType == VoxelDefinition::WallData::MenuType::Tavern) ||
		(menuType == VoxelDefinition::WallData::MenuType::Temple) ||
		(menuType == VoxelDefinition::WallData::MenuType::Tower);
}

bool VoxelDefinition::WallData::menuHasDisplayName(MenuType menuType)
{
	return (menuType == VoxelDefinition::WallData::MenuType::Equipment) ||
		(menuType == VoxelDefinition::WallData::MenuType::MagesGuild) ||
		(menuType == VoxelDefinition::WallData::MenuType::Tavern) ||
		(menuType == VoxelDefinition::WallData::MenuType::Temple);
}

const double VoxelDefinition::ChasmData::WET_LAVA_DEPTH = static_cast<double>(
	INFFile::CeilingData::DEFAULT_HEIGHT) / MIFUtils::ARENA_UNITS;

bool VoxelDefinition::ChasmData::matches(const ChasmData &other) const
{
	return (this->id == other.id) && (this->north == other.north) && (this->east == other.east) &&
		(this->south == other.south) && (this->west == other.west) && (this->type == other.type);
}

bool VoxelDefinition::ChasmData::faceIsVisible(VoxelFacing facing) const
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

int VoxelDefinition::ChasmData::getFaceCount() const
{
	// Assume chasms have floors.
	return 1 + (this->north ? 1 : 0) + (this->east ? 1 : 0) +
		(this->south ? 1 : 0) + (this->west ? 1 : 0);
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

const int VoxelDefinition::TOTAL_IDS = 64;

VoxelDefinition::VoxelDefinition()
{
	// Default to empty.
	this->dataType = VoxelDataType::None;
}

VoxelDefinition VoxelDefinition::makeWall(int sideID, int floorID, int ceilingID,
	const int *menuID, WallData::Type type)
{
	if (sideID >= VoxelDefinition::TOTAL_IDS)
	{
		DebugLogWarning("Wall side ID \"" + std::to_string(sideID) + "\" out of range.");
	}

	if (floorID >= VoxelDefinition::TOTAL_IDS)
	{
		DebugLogWarning("Wall floor ID \"" + std::to_string(floorID) + "\" out of range.");
	}

	if (ceilingID >= VoxelDefinition::TOTAL_IDS)
	{
		DebugLogWarning("Wall ceiling ID \"" + std::to_string(ceilingID) + "\" out of range.");
	}

	VoxelDefinition data;
	data.dataType = VoxelDataType::Wall;

	VoxelDefinition::WallData &wall = data.wall;
	wall.sideID = sideID % VoxelDefinition::TOTAL_IDS;
	wall.floorID = floorID % VoxelDefinition::TOTAL_IDS;
	wall.ceilingID = ceilingID % VoxelDefinition::TOTAL_IDS;

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

VoxelDefinition VoxelDefinition::makeFloor(int id)
{
	if (id >= VoxelDefinition::TOTAL_IDS)
	{
		DebugLogWarning("Floor ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelDefinition data;
	data.dataType = VoxelDataType::Floor;

	VoxelDefinition::FloorData &floor = data.floor;
	floor.id = id % VoxelDefinition::TOTAL_IDS;

	return data;
}

VoxelDefinition VoxelDefinition::makeCeiling(int id)
{
	if (id >= VoxelDefinition::TOTAL_IDS)
	{
		DebugLogWarning("Ceiling ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelDefinition data;
	data.dataType = VoxelDataType::Ceiling;

	VoxelDefinition::CeilingData &ceiling = data.ceiling;
	ceiling.id = id % VoxelDefinition::TOTAL_IDS;

	return data;
}

VoxelDefinition VoxelDefinition::makeRaised(int sideID, int floorID, int ceilingID, double yOffset,
	double ySize, double vTop, double vBottom)
{
	if (sideID >= VoxelDefinition::TOTAL_IDS)
	{
		DebugLogWarning("Raised side ID \"" + std::to_string(sideID) + "\" out of range.");
	}

	if (floorID >= VoxelDefinition::TOTAL_IDS)
	{
		DebugLogWarning("Raised floor ID \"" + std::to_string(floorID) + "\" out of range.");
	}

	if (ceilingID >= VoxelDefinition::TOTAL_IDS)
	{
		DebugLogWarning("Raised ceiling ID \"" + std::to_string(ceilingID) + "\" out of range.");
	}

	VoxelDefinition data;
	data.dataType = VoxelDataType::Raised;

	VoxelDefinition::RaisedData &raised = data.raised;
	raised.sideID = sideID % VoxelDefinition::TOTAL_IDS;
	raised.floorID = floorID % VoxelDefinition::TOTAL_IDS;
	raised.ceilingID = ceilingID % VoxelDefinition::TOTAL_IDS;
	raised.yOffset = yOffset;
	raised.ySize = ySize;
	raised.vTop = vTop;
	raised.vBottom = vBottom;

	return data;
}

VoxelDefinition VoxelDefinition::makeDiagonal(int id, bool type1)
{
	if (id >= VoxelDefinition::TOTAL_IDS)
	{
		DebugLogWarning("Diagonal ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelDefinition data;
	data.dataType = VoxelDataType::Diagonal;

	VoxelDefinition::DiagonalData &diagonal = data.diagonal;
	diagonal.id = id % VoxelDefinition::TOTAL_IDS;
	diagonal.type1 = type1;

	return data;
}

VoxelDefinition VoxelDefinition::makeTransparentWall(int id, bool collider)
{
	if (id >= VoxelDefinition::TOTAL_IDS)
	{
		DebugLogWarning("Transparent wall ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelDefinition data;
	data.dataType = VoxelDataType::TransparentWall;

	VoxelDefinition::TransparentWallData &transparentWall = data.transparentWall;
	transparentWall.id = id % VoxelDefinition::TOTAL_IDS;
	transparentWall.collider = collider;

	return data;
}

VoxelDefinition VoxelDefinition::makeEdge(int id, double yOffset, bool collider,
	bool flipped, VoxelFacing facing)
{
	if (id >= VoxelDefinition::TOTAL_IDS)
	{
		DebugLogWarning("Edge ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelDefinition data;
	data.dataType = VoxelDataType::Edge;

	VoxelDefinition::EdgeData &edge = data.edge;
	edge.id = id % VoxelDefinition::TOTAL_IDS;
	edge.yOffset = yOffset;
	edge.collider = collider;
	edge.flipped = flipped;
	edge.facing = facing;

	return data;
}

VoxelDefinition VoxelDefinition::makeChasm(int id, bool north, bool east, bool south, bool west,
	ChasmData::Type type)
{
	if (id >= VoxelDefinition::TOTAL_IDS)
	{
		DebugLogWarning("Chasm ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelDefinition data;
	data.dataType = VoxelDataType::Chasm;

	VoxelDefinition::ChasmData &chasm = data.chasm;
	chasm.id = id % VoxelDefinition::TOTAL_IDS;
	chasm.north = north;
	chasm.east = east;
	chasm.south = south;
	chasm.west = west;
	chasm.type = type;

	return data;
}

VoxelDefinition VoxelDefinition::makeDoor(int id, DoorData::Type type)
{
	if (id >= VoxelDefinition::TOTAL_IDS)
	{
		DebugLogWarning("Door ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelDefinition data;
	data.dataType = VoxelDataType::Door;

	VoxelDefinition::DoorData &door = data.door;
	door.id = id % VoxelDefinition::TOTAL_IDS;
	door.type = type;

	return data;
}

Double3 VoxelDefinition::getNormal(VoxelFacing facing)
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
