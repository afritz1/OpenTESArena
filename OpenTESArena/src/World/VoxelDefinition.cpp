#include <algorithm>
#include <array>
#include <stdexcept>

#include "VoxelDefinition.h"
#include "VoxelDataType.h"
#include "VoxelFacing3D.h"
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

ArenaTypes::MenuType VoxelDefinition::WallData::getMenuType(int menuID, WorldType worldType)
{
	if (menuID != -1)
	{
		// Mappings of *MENU IDs to city menu types.
		constexpr std::array<std::pair<int, ArenaTypes::MenuType>, 14> CityMenuMappings =
		{
			{
				{ 0, ArenaTypes::MenuType::Equipment },
				{ 1, ArenaTypes::MenuType::Tavern },
				{ 2, ArenaTypes::MenuType::MagesGuild },
				{ 3, ArenaTypes::MenuType::Temple },
				{ 4, ArenaTypes::MenuType::House },
				{ 5, ArenaTypes::MenuType::House },
				{ 6, ArenaTypes::MenuType::House },
				{ 7, ArenaTypes::MenuType::CityGates },
				{ 8, ArenaTypes::MenuType::CityGates },
				{ 9, ArenaTypes::MenuType::Noble },
				{ 10, ArenaTypes::MenuType::None },
				{ 11, ArenaTypes::MenuType::Palace },
				{ 12, ArenaTypes::MenuType::Palace },
				{ 13, ArenaTypes::MenuType::Palace }
			}
		};

		// Mappings of *MENU IDs to wilderness menu types.
		constexpr std::array<std::pair<int, ArenaTypes::MenuType>, 10> WildMenuMappings =
		{
			{
				{ 0, ArenaTypes::MenuType::None },
				{ 1, ArenaTypes::MenuType::Crypt },
				{ 2, ArenaTypes::MenuType::House },
				{ 3, ArenaTypes::MenuType::Tavern },
				{ 4, ArenaTypes::MenuType::Temple },
				{ 5, ArenaTypes::MenuType::Tower },
				{ 6, ArenaTypes::MenuType::CityGates },
				{ 7, ArenaTypes::MenuType::CityGates },
				{ 8, ArenaTypes::MenuType::Dungeon },
				{ 9, ArenaTypes::MenuType::Dungeon }
			}
		};

		// Get the menu type associated with the *MENU ID and world type, or null if there
		// is no mapping (only in exceptional cases). Use a pointer since iterators are tied
		// to their std::array size.
		const ArenaTypes::MenuType *typePtr = [menuID, worldType, &CityMenuMappings,
			&WildMenuMappings]()
		{
			auto getPtr = [menuID](const auto &arr)
			{
				const auto iter = std::find_if(arr.begin(), arr.end(),
					[menuID](const std::pair<int, ArenaTypes::MenuType> &pair)
				{
					return pair.first == menuID;
				});

				return (iter != arr.end()) ? &iter->second : nullptr;
			};

			// Interpretation of *MENU ID depends on whether it's a city or wilderness.
			if (worldType == WorldType::City)
			{
				return getPtr(CityMenuMappings);
			}
			else if (worldType == WorldType::Wilderness)
			{
				return getPtr(WildMenuMappings);
			}
			else
			{
				// @todo: try to replace getPtr() with getIndex() for each world type branch, or
				// just return None menu type.
				throw DebugException("Invalid world type \"" +
					std::to_string(static_cast<int>(worldType)) + "\".");
			}
		}();

		// See if the array contains the associated *MENU ID.
		if (typePtr != nullptr)
		{
			return *typePtr;
		}
		else
		{
			DebugLogWarning("Unrecognized *MENU ID \"" + std::to_string(menuID) + "\".");
			return ArenaTypes::MenuType::None;
		}
	}
	else
	{
		// Not a *MENU block.
		return ArenaTypes::MenuType::None;
	}
}

bool VoxelDefinition::WallData::menuLeadsToInterior(ArenaTypes::MenuType menuType)
{
	return (menuType == ArenaTypes::MenuType::Crypt) ||
		(menuType == ArenaTypes::MenuType::Dungeon) ||
		(menuType == ArenaTypes::MenuType::Equipment) ||
		(menuType == ArenaTypes::MenuType::House) ||
		(menuType == ArenaTypes::MenuType::MagesGuild) ||
		(menuType == ArenaTypes::MenuType::Noble) ||
		(menuType == ArenaTypes::MenuType::Palace) ||
		(menuType == ArenaTypes::MenuType::Tavern) ||
		(menuType == ArenaTypes::MenuType::Temple) ||
		(menuType == ArenaTypes::MenuType::Tower);
}

bool VoxelDefinition::WallData::menuHasDisplayName(ArenaTypes::MenuType menuType)
{
	return (menuType == ArenaTypes::MenuType::Equipment) ||
		(menuType == ArenaTypes::MenuType::MagesGuild) ||
		(menuType == ArenaTypes::MenuType::Tavern) ||
		(menuType == ArenaTypes::MenuType::Temple);
}

const double VoxelDefinition::ChasmData::WET_LAVA_DEPTH = static_cast<double>(
	INFFile::CeilingData::DEFAULT_HEIGHT) / MIFUtils::ARENA_UNITS;

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

const int VoxelDefinition::TOTAL_IDS = 64;

VoxelDefinition::VoxelDefinition()
{
	// Default to empty.
	this->dataType = VoxelDataType::None;
}

VoxelDefinition VoxelDefinition::makeWall(int sideID, int floorID, int ceilingID,
	const std::optional<int> &menuID, WallData::Type type)
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
	bool flipped, VoxelFacing3D facing)
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

VoxelDefinition VoxelDefinition::makeChasm(int id, ChasmData::Type type)
{
	if (id >= VoxelDefinition::TOTAL_IDS)
	{
		DebugLogWarning("Chasm ID \"" + std::to_string(id) + "\" out of range.");
	}

	VoxelDefinition data;
	data.dataType = VoxelDataType::Chasm;

	VoxelDefinition::ChasmData &chasm = data.chasm;
	chasm.id = id % VoxelDefinition::TOTAL_IDS;
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

Double3 VoxelDefinition::getNormal(VoxelFacing3D facing)
{
	// Decide what the normal is, based on the facing.
	if (facing == VoxelFacing3D::PositiveX)
	{
		return Double3::UnitX;
	}
	else if (facing == VoxelFacing3D::NegativeX)
	{
		return -Double3::UnitX;
	}
	else if (facing == VoxelFacing3D::PositiveZ)
	{
		return Double3::UnitZ;
	}
	else
	{
		return -Double3::UnitZ;
	}
}

bool VoxelDefinition::allowsChasmFace() const
{
	return (this->dataType != VoxelDataType::None) && (this->dataType != VoxelDataType::Chasm);
}
