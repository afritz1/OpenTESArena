#include <algorithm>
#include <cmath>

#include "CityDataFile.h"
#include "ExeData.h"
#include "MiscAssets.h"
#include "../Math/Random.h"
#include "../Utilities/Bytes.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"
#include "../World/Location.h"
#include "../World/LocationType.h"
#include "../World/VoxelData.h"

#include "components/vfs/manager.hpp"

bool CityDataFile::ProvinceData::LocationData::isVisible() const
{
	return (this->visibility & 0x2) != 0;
}

void CityDataFile::ProvinceData::LocationData::setVisible(bool visible)
{
	this->visibility = visible ? 0x2 : 0;
}

Rect CityDataFile::ProvinceData::getGlobalRect() const
{
	return Rect(this->globalX, this->globalY, this->globalW, this->globalH);
}

const CityDataFile::ProvinceData::LocationData &CityDataFile::ProvinceData::getLocationData(
	int locationID) const
{
	if (locationID < 8)
	{
		// City.
		return this->cityStates.at(locationID);
	}
	else if (locationID < 16)
	{
		// Town.
		return this->towns.at(locationID - 8);
	}
	else if (locationID < 32)
	{
		// Village.
		return this->villages.at(locationID - 16);
	}
	else if (locationID == 32)
	{
		// Staff dungeon.
		return this->secondDungeon;
	}
	else if (locationID == 33)
	{
		// Staff map dungeon.
		return this->firstDungeon;
	}
	else if (locationID < 48)
	{
		// Named dungeon.
		return this->randomDungeons.at(locationID - 34);
	}
	else
	{
		throw DebugException("Bad location ID \"" +
			std::to_string(locationID) + "\".");
	}
}

const int CityDataFile::PROVINCE_COUNT = 9;

CityDataFile::ProvinceData &CityDataFile::getProvinceData(int index)
{
	return this->provinces.at(index);
}

const CityDataFile::ProvinceData &CityDataFile::getProvinceData(int index) const
{
	return this->provinces.at(index);
}

int CityDataFile::getGlobalCityID(int localCityID, int provinceID)
{
	return (provinceID << 5) + localCityID;
}

std::pair<int, int> CityDataFile::getLocalCityAndProvinceID(int globalCityID)
{
	return std::make_pair(globalCityID & 0x1F, globalCityID >> 5);
}

int CityDataFile::getDistance(const Int2 &globalSrc, const Int2 &globalDst)
{
	const int dx = std::abs(globalSrc.x - globalDst.x);
	const int dy = std::abs(globalSrc.y - globalDst.y);
	return std::max(dx, dy) + (std::min(dx, dy) / 4);
}

Int2 CityDataFile::localPointToGlobal(const Int2 &localPoint, const Rect &rect)
{
	const int globalX = ((localPoint.x * ((rect.getWidth() * 100) / 320)) / 100) + rect.getLeft();
	const int globalY = ((localPoint.y * ((rect.getHeight() * 100) / 200)) / 100) + rect.getTop();
	return Int2(globalX, globalY);
}

Int2 CityDataFile::globalPointToLocal(const Int2 &globalPoint, const Rect &rect)
{
	const int localX = ((globalPoint.x - rect.getLeft()) * 100) / ((rect.getWidth() * 100) / 320);
	const int localY = ((globalPoint.y - rect.getTop()) * 100) / ((rect.getHeight() * 100) / 200);
	return Int2(localX, localY);
}

std::string CityDataFile::getMainQuestDungeonMifName(uint32_t seed)
{
	const std::string seedString = std::to_string(seed);
	const std::string mifName = seedString.substr(0, 8) + ".MIF";
	return mifName;
}

uint16_t CityDataFile::getDoorVoxelOffset(int x, int y)
{
	return (y << 8) + (x << 1);
}

std::string CityDataFile::getDoorVoxelMifName(int x, int y, int menuID,
	int localCityID, int provinceID, bool isCity, const ExeData &exeData) const
{
	// Get the menu type associated with the *MENU ID.
	const VoxelData::WallData::MenuType menuType =
		VoxelData::WallData::getMenuType(menuID, isCity);

	// Check special case first: if it's a palace block in the center province's city,
	// the .MIF name is hardcoded.
	if ((menuType == VoxelData::WallData::MenuType::Palace) &&
		(provinceID == 8) && (localCityID == 0))
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
			const std::array<std::pair<VoxelData::WallData::MenuType, int>, 12> MenuMifMappings =
			{
				{
					{ VoxelData::WallData::MenuType::CityGates, NO_INDEX },
					{ VoxelData::WallData::MenuType::Crypt, 7 },
					{ VoxelData::WallData::MenuType::Dungeon, NO_INDEX },
					{ VoxelData::WallData::MenuType::Equipment, 5 },
					{ VoxelData::WallData::MenuType::House, 1 },
					{ VoxelData::WallData::MenuType::MagesGuild, 6 },
					{ VoxelData::WallData::MenuType::Noble, 2 },
					{ VoxelData::WallData::MenuType::None, NO_INDEX },
					{ VoxelData::WallData::MenuType::Palace, 0 },
					{ VoxelData::WallData::MenuType::Tavern, 3 },
					{ VoxelData::WallData::MenuType::Temple, 4 },
					{ VoxelData::WallData::MenuType::Tower, 10 }
				}
			};

			// See if the given menu type has a .MIF prefix mapping.
			const auto iter = std::find_if(MenuMifMappings.begin(), MenuMifMappings.end(),
				[menuType](const std::pair<VoxelData::WallData::MenuType, int> &pair)
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
						if (menuType == VoxelData::WallData::MenuType::Palace)
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
								throw DebugException("Invalid location type \"" +
									std::to_string(static_cast<int>(locationType)) + "\".");
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
				throw DebugException("Bad menu type \"" +
					std::to_string(static_cast<int>(menuType)) + "\".");
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
		const uint16_t offset = CityDataFile::getDoorVoxelOffset(x, y);
		const uint32_t rulerSeed = this->getRulerSeed(localCityID, provinceID);

		// Decide which variant of the interior to use.
		const int variantID = [rulerSeed, offset, menuType]()
		{
			// Palaces have fewer .MIF files to choose from, and their variant depends
			// on the ruler seed. Although there are five city-state palace .MIF files,
			// only three of them are used.
			const bool isPalace = menuType == VoxelData::WallData::MenuType::Palace;
			const int palaceCount = 3;
			return isPalace ? (((rulerSeed >> 8) & 0xFFFF) % palaceCount) :
				((Bytes::ror(offset, 4) ^ offset) % 8);
		}();

		// Generate .MIF filename.
		return menuName + std::to_string(variantID + 1) + ".MIF";
	}
}

int CityDataFile::getDoorVoxelLockLevel(int x, int y, ArenaRandom &random)
{
	const uint16_t offset = CityDataFile::getDoorVoxelOffset(x, y);
	const uint32_t seed = offset + (offset << 16);
	random.srand(seed);
	return (random.next() % 10) + 1; // 0..9 + 1.
}

int CityDataFile::getCityDimensions(LocationType locationType)
{
	if (locationType == LocationType::CityState)
	{
		return 6;
	}
	else if (locationType == LocationType::Town)
	{
		return 5;
	}
	else if (locationType == LocationType::Village)
	{
		return 4;
	}
	else
	{
		throw DebugException("Bad location type \"" +
			std::to_string(static_cast<int>(locationType)) + "\".");
	}
}

int CityDataFile::getCityTemplateCount(bool isCoastal, bool isCityState)
{
	return isCoastal ? (isCityState ? 3 : 2) : 5;
}

int CityDataFile::getCityTemplateNameIndex(LocationType locationType, bool isCoastal)
{
	if (locationType == LocationType::CityState)
	{
		return isCoastal ? 5 : 4;
	}
	else if (locationType == LocationType::Town)
	{
		return isCoastal ? 1 : 0;
	}
	else if (locationType == LocationType::Village)
	{
		return isCoastal ? 3 : 2;
	}
	else
	{
		throw DebugException("Bad location type \"" +
			std::to_string(static_cast<int>(locationType)) + "\".");
	}
}

int CityDataFile::getCityStartingPositionIndex(LocationType locationType,
	bool isCoastal, int templateID)
{
	if (locationType == LocationType::CityState)
	{
		return isCoastal ? (19 + templateID) : (14 + templateID);
	}
	else if (locationType == LocationType::Town)
	{
		return isCoastal ? (5 + templateID) : templateID;
	}
	else if (locationType == LocationType::Village)
	{
		return isCoastal ? (12 + templateID) : (7 + templateID);
	}
	else
	{
		throw DebugException("Bad location type \"" +
			std::to_string(static_cast<int>(locationType)) + "\".");
	}
}

int CityDataFile::getCityReservedBlockListIndex(bool isCoastal, int templateID)
{
	return isCoastal ? (5 + templateID) : templateID;
}

int CityDataFile::getServiceSaveFileNumber(int doorX, int doorY)
{
	return (doorY << 8) + doorX;
}

int CityDataFile::getWildernessServiceSaveFileNumber(int wildX, int wildY)
{
	return (wildY << 16) + wildX;
}

int CityDataFile::getGlobalQuarter(const Int2 &globalPoint) const
{
	Rect provinceRect;

	// Find the province that contains the global point.
	const auto iter = std::find_if(this->provinces.begin(), this->provinces.end(),
		[&globalPoint, &provinceRect](const CityDataFile::ProvinceData &province)
	{
		provinceRect = province.getGlobalRect();
		return provinceRect.containsInclusive(globalPoint);
	});

	DebugAssertMsg(iter != this->provinces.end(), "No matching province for global point (" +
		std::to_string(globalPoint.x) + ", " + std::to_string(globalPoint.y) + ").");

	const Int2 localPoint = CityDataFile::globalPointToLocal(globalPoint, provinceRect);
	const int provinceID = static_cast<int>(std::distance(this->provinces.begin(), iter));

	// Get the global quarter index.
	const int globalQuarter = [&localPoint, provinceID]()
	{
		int index = provinceID * 4;
		const bool inRightHalf = localPoint.x >= 160;
		const bool inBottomHalf = localPoint.y >= 100;

		// Add to the index depending on which quadrant the local point is in.
		if (inRightHalf)
		{
			index++;
		}

		if (inBottomHalf)
		{
			index += 2;
		}

		return index;
	}();

	return globalQuarter;
}

int CityDataFile::getTravelDays(int startLocationID, int startProvinceID, int endLocationID,
	int endProvinceID, int month, const std::array<WeatherType, 36> &weathers,
	ArenaRandom &random, const MiscAssets &miscAssets) const
{
	auto getGlobalPoint = [this](int locationID, int provinceID)
	{
		const auto &province = this->getProvinceData(provinceID);
		const auto &location = province.getLocationData(locationID);
		return CityDataFile::localPointToGlobal(
			Int2(location.x, location.y), province.getGlobalRect());
	};

	// The two world map points to calculate between.
	const Int2 startGlobalPoint = getGlobalPoint(startLocationID, startProvinceID);
	const Int2 endGlobalPoint = getGlobalPoint(endLocationID, endProvinceID);

	// Get all the points along the line between the two points.
	const std::vector<Int2> points = Int2::bresenhamLine(startGlobalPoint, endGlobalPoint);

	int totalTime = 0;
	for (const Int2 &point : points)
	{
		const int monthIndex = (month + (totalTime / 3000)) % 12;
		const int weatherIndex = [this, &weathers, &point]()
		{
			// Find which province quarter the global point is in.
			const int quarterIndex = this->getGlobalQuarter(point);

			// Convert the weather type to its equivalent index.
			return static_cast<int>(weathers.at(quarterIndex));
		}();

		// The type of terrain at the world map point.
		const auto &worldMapTerrain = miscAssets.getWorldMapTerrain();
		const uint8_t terrainIndex = MiscAssets::WorldMapTerrain::getNormalizedIndex(
			worldMapTerrain.getAt(point.x, point.y));

		// Calculate the travel speed based on climate and weather.
		const auto &exeData = miscAssets.getExeData();
		const auto &climateSpeedTables = exeData.locations.climateSpeedTables;
		const auto &weatherSpeedTables = exeData.locations.weatherSpeedTables;
		const int climateSpeed = climateSpeedTables.at(terrainIndex).at(monthIndex);
		const int weatherMod = [terrainIndex, weatherIndex, &weatherSpeedTables]()
		{
			const int weatherSpeed = weatherSpeedTables.at(terrainIndex).at(weatherIndex);

			// Special case: 0 equals 100.
			return (weatherSpeed == 0) ? 100 : weatherSpeed;
		}();

		const int travelSpeed = (climateSpeed * weatherMod) / 100;

		// Add the pixel's travel time onto the total time.
		const int pixelTravelTime = 2000 / travelSpeed;
		totalTime += pixelTravelTime;
	}

	// Calculate the actual travel days based on the total time.
	const int travelDays = [&random, totalTime]()
	{
		const int minDays = 1;
		const int maxDays = 2000;
		int days = std::clamp(totalTime / 100, minDays, maxDays);

		if (days > 20)
		{
			days += (random.next() % 10) - 5;
		}

		return days;
	}();

	return travelDays;
}

uint32_t CityDataFile::getCitySeed(int localCityID, int provinceID) const
{
	const auto &province = this->getProvinceData(provinceID);
	const int locationID = Location::cityToLocationID(localCityID);
	const auto &location = province.getLocationData(locationID);
	return static_cast<uint32_t>((location.x << 16) + location.y);
}

uint32_t CityDataFile::getWildernessSeed(int localCityID, int provinceID) const
{
	const auto &province = this->getProvinceData(provinceID);
	const auto &location = province.getLocationData(Location::cityToLocationID(localCityID));
	const std::string &locationName = location.name;
	DebugAssertMsg(locationName.size() >= 4, "Name \"" + locationName + "\" too short.");

	// Use the first four letters as the seed.
	const uint8_t *ptr = reinterpret_cast<const uint8_t*>(locationName.data());
	return Bytes::getLE32(ptr);
}

uint32_t CityDataFile::getRulerSeed(int localCityID, int provinceID) const
{
	const Int2 globalPoint = this->getGlobalPoint(localCityID, provinceID);
	const uint32_t seed = static_cast<uint32_t>((globalPoint.x << 16) + globalPoint.y);
	return Bytes::rol(seed, 16);
}

uint32_t CityDataFile::getDistantSkySeed(int localCityID, int provinceID) const
{
	const Int2 globalPoint = this->getGlobalPoint(localCityID, provinceID);
	const uint32_t seed = static_cast<uint32_t>((globalPoint.x << 16) + globalPoint.y);
	return seed * provinceID;
}

uint32_t CityDataFile::getDungeonSeed(int localDungeonID, int provinceID) const
{
	const auto &province = this->provinces.at(provinceID);
	const auto &dungeon = [localDungeonID, &province]()
	{
		if (localDungeonID == 0)
		{
			// Second main quest dungeon.
			return province.secondDungeon;
		}
		else if (localDungeonID == 1)
		{
			// First main quest dungeon.
			return province.firstDungeon;
		}
		else
		{
			return province.randomDungeons.at(localDungeonID - 2);
		}
	}();

	const uint32_t seed = (dungeon.y << 16) + dungeon.x + provinceID;
	return (~Bytes::rol(seed, 5)) & 0xFFFFFFFF;
}

uint32_t CityDataFile::getWildernessDungeonSeed(int provinceID,
	int wildBlockX, int wildBlockY) const
{
	const auto &province = this->provinces.at(provinceID);
	const uint32_t baseSeed = ((province.globalX << 16) + province.globalY) * provinceID;
	return (baseSeed + (((wildBlockY << 6) + wildBlockX) & 0xFFFF)) & 0xFFFFFFFF;
}

Int2 CityDataFile::getGlobalPoint(int localCityID, int provinceID) const
{
	const auto &province = this->getProvinceData(provinceID);
	const int locationID = Location::cityToLocationID(localCityID);
	const auto &location = province.getLocationData(locationID);
	const Int2 localPoint(location.x, location.y);
	const Int2 globalPoint = CityDataFile::localPointToGlobal(
		localPoint, province.getGlobalRect());
	return globalPoint;
}

Int2 CityDataFile::getLocalCityPoint(uint32_t citySeed)
{
	return Int2(citySeed >> 16, citySeed & 0xFFFF);
}

void CityDataFile::init(const std::string &filename)
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
	DebugAssertMsg(stream != nullptr, "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	std::vector<uint8_t> srcData(stream->tellg());
	stream->seekg(0, std::ios::beg);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// Iterate over each province and initialize the location data.
	for (size_t i = 0; i < this->provinces.size(); i++)
	{
		auto &province = this->provinces.at(i);

		// Size of each province definition in bytes.
		const size_t provinceDataSize = 1228;
		const size_t startOffset = provinceDataSize * i;

		// Max number of characters in a province/location name (including null terminator).
		const size_t nameSize = 20;

		// Read the province header.
		const char *provinceNamePtr = reinterpret_cast<const char*>(srcData.data() + startOffset);
		province.name = std::string(provinceNamePtr);

		const uint8_t *provinceGlobalDimsPtr =
			reinterpret_cast<const uint8_t*>(provinceNamePtr + nameSize);
		province.globalX = Bytes::getLE16(provinceGlobalDimsPtr);
		province.globalY = Bytes::getLE16(provinceGlobalDimsPtr + sizeof(uint16_t));
		province.globalW = Bytes::getLE16(provinceGlobalDimsPtr + (sizeof(uint16_t) * 2));
		province.globalH = Bytes::getLE16(provinceGlobalDimsPtr + (sizeof(uint16_t) * 3));

		const uint8_t *locationPtr = provinceGlobalDimsPtr + (sizeof(uint16_t) * 4);

		// Lambda for initializing the given location and pushing the location pointer forward.
		auto initLocation = [nameSize, &locationPtr](
			CityDataFile::ProvinceData::LocationData &locationData)
		{
			const char *locationNamePtr = reinterpret_cast<const char*>(locationPtr);
			locationData.name = std::string(locationNamePtr);

			const uint8_t *locationDimsPtr = locationPtr + nameSize;
			locationData.x = Bytes::getLE16(locationDimsPtr);
			locationData.y = Bytes::getLE16(locationDimsPtr + sizeof(uint16_t));
			locationData.visibility = *(locationDimsPtr + (sizeof(uint16_t) * 2));

			// Size of each location definition in bytes.
			const size_t locationDataSize = 25;
			locationPtr += locationDataSize;
		};

		for (auto &cityState : province.cityStates)
		{
			// Read the city-state data.
			initLocation(cityState);
		}

		for (auto &town : province.towns)
		{
			// Read the town data.
			initLocation(town);
		}

		for (auto &village : province.villages)
		{
			// Read the village data.
			initLocation(village);
		}

		// Read the dungeon data. The second dungeon is listed first.
		initLocation(province.secondDungeon);
		initLocation(province.firstDungeon);

		// Read random dungeon data.
		for (auto &dungeon : province.randomDungeons)
		{
			// Read the random dungeon data.
			initLocation(dungeon);
		}
	}
}
