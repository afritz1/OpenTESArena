#include <algorithm>

#include "LevelUtils.h"
#include "Location.h"
#include "LocationType.h"
#include "LocationUtils.h"
#include "VoxelDefinition.h"
#include "../Assets/ExeData.h"
#include "../Math/Random.h"

#include "components/utilities/Bytes.h"
#include "components/debug/Debug.h"
#include "components/utilities/String.h"

uint16_t LevelUtils::getDoorVoxelOffset(int x, int y)
{
	return (y << 8) + (x << 1);
}

std::string LevelUtils::getDoorVoxelMifName(int x, int y, int menuID, int localCityID,
	int provinceID, const CityDataFile::ProvinceData &province, bool isCity, const ExeData &exeData)
{
	// Get the menu type associated with the *MENU ID.
	const VoxelDefinition::WallData::MenuType menuType =
		VoxelDefinition::WallData::getMenuType(menuID, isCity);

	// Check special case first: if it's a palace block in the center province's city,
	// the .MIF name is hardcoded.
	if ((menuType == VoxelDefinition::WallData::MenuType::Palace) &&
		(provinceID == Location::CENTER_PROVINCE_ID) && (localCityID == 0))
	{
		return String::toUppercase(exeData.locations.finalDungeonMifName);
	}

	// Get the prefix associated with the menu type.
	const std::string menuName = [localCityID, &exeData, menuType]()
	{
		const LocationType locationType = Location::getCityType(localCityID);

		const std::string name = [&exeData, locationType, menuType]() -> std::string
		{
			// Mappings of menu types to menu .MIF prefix indices. Menus that have no .MIF
			// filename mapping are considered special cases. TOWNPAL and VILPAL are not used
			// since the palace type can be deduced from the current city type.
			const int NO_INDEX = -1;
			const std::array<std::pair<VoxelDefinition::WallData::MenuType, int>, 12> MenuMifMappings =
			{
				{
					{ VoxelDefinition::WallData::MenuType::CityGates, NO_INDEX },
					{ VoxelDefinition::WallData::MenuType::Crypt, 7 },
					{ VoxelDefinition::WallData::MenuType::Dungeon, NO_INDEX },
					{ VoxelDefinition::WallData::MenuType::Equipment, 5 },
					{ VoxelDefinition::WallData::MenuType::House, 1 },
					{ VoxelDefinition::WallData::MenuType::MagesGuild, 6 },
					{ VoxelDefinition::WallData::MenuType::Noble, 2 },
					{ VoxelDefinition::WallData::MenuType::None, NO_INDEX },
					{ VoxelDefinition::WallData::MenuType::Palace, 0 },
					{ VoxelDefinition::WallData::MenuType::Tavern, 3 },
					{ VoxelDefinition::WallData::MenuType::Temple, 4 },
					{ VoxelDefinition::WallData::MenuType::Tower, 10 }
				}
			};

			// See if the given menu type has a .MIF prefix mapping.
			const auto iter = std::find_if(MenuMifMappings.begin(), MenuMifMappings.end(),
				[menuType](const std::pair<VoxelDefinition::WallData::MenuType, int> &pair)
			{
				return pair.first == menuType;
			});

			if (iter != MenuMifMappings.end())
			{
				const int index = iter->second;

				if (index != NO_INDEX)
				{
					// Get the menu's .MIF prefix index. If it's a palace, then decide which palace
					// prefix to use based on the location type.
					const int menuMifIndex = [locationType, menuType, index]()
					{
						if (menuType == VoxelDefinition::WallData::MenuType::Palace)
						{
							if (locationType == LocationType::CityState)
							{
								return 0;
							}
							else if (locationType == LocationType::Town)
							{
								return 8;
							}
							else if (locationType == LocationType::Village)
							{
								return 9;
							}
							else
							{
								DebugUnhandledReturnMsg(int,
									std::to_string(static_cast<int>(locationType)));
							}
						}
						else
						{
							return index;
						}
					}();

					const auto &prefixes = exeData.locations.menuMifPrefixes;
					return prefixes.at(menuMifIndex);
				}
				else
				{
					// The menu has no valid .MIF prefix.
					return std::string();
				}
			}
			else
			{
				DebugUnhandledReturnMsg(std::string, std::to_string(static_cast<int>(menuType)));
			}
		}();

		return String::toUppercase(name);
	}();

	// Some menu names don't map to an actual building type, so they are special cases
	// and should be ignored by the caller.
	const bool isSpecialCase = menuName.size() == 0;

	if (isSpecialCase)
	{
		// No .MIF filename. The caller knows not to try and load a .MIF file.
		return std::string();
	}
	else
	{
		// Offset is based on X and Y position in world; used with variant calculation.
		const uint16_t offset = LevelUtils::getDoorVoxelOffset(x, y);
		const uint32_t rulerSeed = [localCityID, &province]()
		{
			const auto &location = province.getLocationData(localCityID);
			const Int2 localPoint(location.x, location.y);
			return LocationUtils::getRulerSeed(localPoint, province.getGlobalRect());
		}();

		// Decide which variant of the interior to use.
		const int variantID = [rulerSeed, offset, menuType]()
		{
			// Palaces have fewer .MIF files to choose from, and their variant depends
			// on the ruler seed. Although there are five city-state palace .MIF files,
			// only three of them are used.
			const bool isPalace = menuType == VoxelDefinition::WallData::MenuType::Palace;
			const int palaceCount = 3;
			return isPalace ? (((rulerSeed >> 8) & 0xFFFF) % palaceCount) :
				((Bytes::ror(offset, 4) ^ offset) % 8);
		}();

		// Generate .MIF filename.
		return menuName + std::to_string(variantID + 1) + ".MIF";
	}
}

int LevelUtils::getDoorVoxelLockLevel(int x, int y, ArenaRandom &random)
{
	const uint16_t offset = LevelUtils::getDoorVoxelOffset(x, y);
	const uint32_t seed = offset + (offset << 16);
	random.srand(seed);
	return (random.next() % 10) + 1; // 0..9 + 1.
}

int LevelUtils::getServiceSaveFileNumber(int doorX, int doorY)
{
	return (doorY << 8) + doorX;
}

int LevelUtils::getWildernessServiceSaveFileNumber(int wildX, int wildY)
{
	return (wildY << 16) + wildX;
}
