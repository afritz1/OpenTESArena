#include <algorithm>
#include <vector>

#include "ArenaVoxelUtils.h"
#include "../World/MapType.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

ArenaTypes::MenuType ArenaVoxelUtils::getMenuType(int menuID, MapType mapType)
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
		const ArenaTypes::MenuType *typePtr = [menuID, mapType, &CityMenuMappings,
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
			if (mapType == MapType::City)
			{
				return getPtr(CityMenuMappings);
			}
			else if (mapType == MapType::Wilderness)
			{
				return getPtr(WildMenuMappings);
			}
			else
			{
				// @todo: try to replace getPtr() with getIndex() for each world type branch, or
				// just return None menu type.
				throw DebugException("Invalid world type \"" +
					std::to_string(static_cast<int>(mapType)) + "\".");
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

bool ArenaVoxelUtils::menuLeadsToInterior(ArenaTypes::MenuType menuType)
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

bool ArenaVoxelUtils::isCityGateMenuIndex(int menuIndex, MapType mapType)
{
	if (mapType == MapType::Interior)
	{
		// No city gates in interiors.
		return false;
	}
	else if (mapType == MapType::City)
	{
		return (menuIndex == 7) || (menuIndex == 8);
	}
	else if (mapType == MapType::Wilderness)
	{
		return (menuIndex == 6) || (menuIndex == 7);
	}
	else
	{
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(mapType)));
	}
}

bool ArenaVoxelUtils::menuHasDisplayName(ArenaTypes::MenuType menuType)
{
	return (menuType == ArenaTypes::MenuType::Equipment) ||
		(menuType == ArenaTypes::MenuType::MagesGuild) ||
		(menuType == ArenaTypes::MenuType::Tavern) ||
		(menuType == ArenaTypes::MenuType::Temple);
}

int ArenaVoxelUtils::clampVoxelTextureID(int id)
{
	if (id >= ArenaVoxelUtils::TOTAL_VOXEL_IDS)
	{
		DebugLogWarning("Original voxel texture ID \"" + std::to_string(id) + "\" out of range.");
		id %= ArenaVoxelUtils::TOTAL_VOXEL_IDS;
	}

	return id;
}

std::string ArenaVoxelUtils::getVoxelTextureFilename(int id, const INFFile &inf)
{
	const BufferView<const INFVoxelTexture> voxelTextures = inf.getVoxelTextures();
	if ((id < 0) || (id >= voxelTextures.getCount()))
	{
		DebugLogWarningFormat("Invalid .INF voxel texture ID \"%d\", defaulting to filename at index 0.", id);
		id = 0;
	}

	const INFVoxelTexture &textureData = voxelTextures[id];
	const char *filename = textureData.filename.c_str();
	return String::toUppercase(filename);
}

std::optional<int> ArenaVoxelUtils::getVoxelTextureSetIndex(int id, const INFFile &inf)
{
	const BufferView<const INFVoxelTexture> voxelTextures = inf.getVoxelTextures();
	if ((id < 0) || (id >= voxelTextures.getCount()))
	{
		DebugLogWarningFormat("Invalid .INF voxel texture ID \"%d\", defaulting to .SET index at index 0.", id);
		id = 0;
	}

	const INFVoxelTexture &textureData = voxelTextures[id];
	return textureData.setIndex;
}

bool ArenaVoxelUtils::isFloorWildWallColored(int floorID, MapType mapType)
{
	if (mapType != MapType::Wilderness)
	{
		return false;
	}

	return (floorID != 0) && (floorID != 2) && (floorID != 3) && (floorID != 4);
}

std::optional<int> ArenaVoxelUtils::tryGetOpenSoundIndex(ArenaTypes::DoorType type)
{
	if (type == ArenaTypes::DoorType::Swinging)
	{
		return 6;
	}
	else if (type == ArenaTypes::DoorType::Sliding)
	{
		return 14;
	}
	else if (type == ArenaTypes::DoorType::Raising)
	{
		return 15;
	}
	else
	{
		return std::nullopt;
	}
}

std::optional<int> ArenaVoxelUtils::tryGetCloseSoundIndex(ArenaTypes::DoorType type)
{
	if (type == ArenaTypes::DoorType::Swinging)
	{
		return 5;
	}
	else if (type == ArenaTypes::DoorType::Sliding)
	{
		return 14;
	}
	else if (type == ArenaTypes::DoorType::Raising)
	{
		return 15;
	}
	else
	{
		return std::nullopt;
	}
}

bool ArenaVoxelUtils::doorHasSoundOnClosed(ArenaTypes::DoorType type)
{
	return type == ArenaTypes::DoorType::Swinging;
}

bool ArenaVoxelUtils::doorHasSoundOnClosing(ArenaTypes::DoorType type)
{
	return (type == ArenaTypes::DoorType::Sliding) || (type == ArenaTypes::DoorType::Raising) ||
		(type == ArenaTypes::DoorType::Splitting);
}
