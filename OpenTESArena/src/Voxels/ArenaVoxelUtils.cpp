#include <algorithm>
#include <vector>

#include "ArenaVoxelUtils.h"
#include "../World/MapType.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

ArenaMenuType ArenaVoxelUtils::getMenuType(int menuID, MapType mapType)
{
	if (menuID != -1)
	{
		// Mappings of *MENU IDs to city menu types.
		constexpr std::array<std::pair<int, ArenaMenuType>, 14> CityMenuMappings =
		{
			{
				{ 0, ArenaMenuType::Equipment },
				{ 1, ArenaMenuType::Tavern },
				{ 2, ArenaMenuType::MagesGuild },
				{ 3, ArenaMenuType::Temple },
				{ 4, ArenaMenuType::House },
				{ 5, ArenaMenuType::House },
				{ 6, ArenaMenuType::House },
				{ 7, ArenaMenuType::CityGates },
				{ 8, ArenaMenuType::CityGates },
				{ 9, ArenaMenuType::Noble },
				{ 10, ArenaMenuType::None },
				{ 11, ArenaMenuType::Palace },
				{ 12, ArenaMenuType::Palace },
				{ 13, ArenaMenuType::Palace }
			}
		};

		// Mappings of *MENU IDs to wilderness menu types.
		constexpr std::array<std::pair<int, ArenaMenuType>, 10> WildMenuMappings =
		{
			{
				{ 0, ArenaMenuType::None },
				{ 1, ArenaMenuType::Crypt },
				{ 2, ArenaMenuType::House },
				{ 3, ArenaMenuType::Tavern },
				{ 4, ArenaMenuType::Temple },
				{ 5, ArenaMenuType::Tower },
				{ 6, ArenaMenuType::CityGates },
				{ 7, ArenaMenuType::CityGates },
				{ 8, ArenaMenuType::Dungeon },
				{ 9, ArenaMenuType::Dungeon }
			}
		};

		// Get the menu type associated with the *MENU ID and world type, or null if there
		// is no mapping (only in exceptional cases). Use a pointer since iterators are tied
		// to their std::array size.
		const ArenaMenuType *typePtr = [menuID, mapType, &CityMenuMappings,
			&WildMenuMappings]()
		{
			auto getPtr = [menuID](const auto &arr)
			{
				const auto iter = std::find_if(arr.begin(), arr.end(),
					[menuID](const std::pair<int, ArenaMenuType> &pair)
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
			return ArenaMenuType::None;
		}
	}
	else
	{
		// Not a *MENU block.
		return ArenaMenuType::None;
	}
}

bool ArenaVoxelUtils::menuLeadsToInterior(ArenaMenuType menuType)
{
	return (menuType == ArenaMenuType::Crypt) ||
		(menuType == ArenaMenuType::Dungeon) ||
		(menuType == ArenaMenuType::Equipment) ||
		(menuType == ArenaMenuType::House) ||
		(menuType == ArenaMenuType::MagesGuild) ||
		(menuType == ArenaMenuType::Noble) ||
		(menuType == ArenaMenuType::Palace) ||
		(menuType == ArenaMenuType::Tavern) ||
		(menuType == ArenaMenuType::Temple) ||
		(menuType == ArenaMenuType::Tower);
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

bool ArenaVoxelUtils::menuHasDisplayName(ArenaMenuType menuType)
{
	return (menuType == ArenaMenuType::Equipment) ||
		(menuType == ArenaMenuType::MagesGuild) ||
		(menuType == ArenaMenuType::Tavern) ||
		(menuType == ArenaMenuType::Temple);
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

std::optional<int> ArenaVoxelUtils::tryGetOpenSoundIndex(ArenaDoorType type)
{
	if (type == ArenaDoorType::Swinging)
	{
		return 6;
	}
	else if (type == ArenaDoorType::Sliding)
	{
		return 14;
	}
	else if (type == ArenaDoorType::Raising)
	{
		return 15;
	}
	else
	{
		return std::nullopt;
	}
}

std::optional<int> ArenaVoxelUtils::tryGetCloseSoundIndex(ArenaDoorType type)
{
	if (type == ArenaDoorType::Swinging)
	{
		return 5;
	}
	else if (type == ArenaDoorType::Sliding)
	{
		return 14;
	}
	else if (type == ArenaDoorType::Raising)
	{
		return 15;
	}
	else
	{
		return std::nullopt;
	}
}

bool ArenaVoxelUtils::doorHasSoundOnClosed(ArenaDoorType type)
{
	return type == ArenaDoorType::Swinging;
}

bool ArenaVoxelUtils::doorHasSoundOnClosing(ArenaDoorType type)
{
	return (type == ArenaDoorType::Sliding) || (type == ArenaDoorType::Raising) ||
		(type == ArenaDoorType::Splitting);
}
