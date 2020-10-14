#include <algorithm>
#include <cmath>

#include "LocationType.h"
#include "LocationUtils.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Math/MathUtils.h"
#include "../Math/Random.h"
#include "../Math/Vector2.h"

#include "components/debug/Debug.h"
#include "components/utilities/Bytes.h"

namespace LocationUtils
{
	// Parent function for getting the climate type of a location.
	ClimateType getClimateType(int locationID, int provinceID,
		const BinaryAssetLibrary &binaryAssetLibrary)
	{
		const auto &cityData = binaryAssetLibrary.getCityDataFile();
		const auto &province = cityData.getProvinceData(provinceID);
		const auto &location = province.getLocationData(locationID);
		const Int2 localPoint(location.x, location.y);
		const Int2 globalPoint = LocationUtils::getGlobalPoint(localPoint, province.getGlobalRect());
		const auto &worldMapTerrain = binaryAssetLibrary.getWorldMapTerrain();
		const uint8_t terrain = worldMapTerrain.getFailSafeAt(globalPoint.x, globalPoint.y);
		return BinaryAssetLibrary::WorldMapTerrain::toClimateType(terrain);
	}
}

int LocationUtils::cityToLocationID(int localCityID)
{
	return localCityID;
}

int LocationUtils::dungeonToLocationID(int localDungeonID)
{
	return localDungeonID + 32;
}

int LocationUtils::getGlobalCityID(int localCityID, int provinceID)
{
	return (provinceID << 5) + localCityID;
}

std::pair<int, int> LocationUtils::getLocalCityAndProvinceID(int globalCityID)
{
	return std::make_pair(globalCityID & 0x1F, globalCityID >> 5);
}

LocationType LocationUtils::getCityType(int localCityID)
{
	if (localCityID < 8)
	{
		return LocationType::CityState;
	}
	else if (localCityID < 16)
	{
		return LocationType::Town;
	}
	else if (localCityID < 32)
	{
		return LocationType::Village;
	}
	else
	{
		DebugUnhandledReturnMsg(LocationType, std::to_string(localCityID));
	}
}

LocationType LocationUtils::getDungeonType(int localDungeonID)
{
	if (localDungeonID == 0)
	{
		return LocationType::StaffDungeon;
	}
	else if (localDungeonID == 1)
	{
		return LocationType::StaffMapDungeon;
	}
	else
	{
		return LocationType::NamedDungeon;
	}
}

ClimateType LocationUtils::getCityClimateType(int localCityID, int provinceID,
	const BinaryAssetLibrary &binaryAssetLibrary)
{
	const int locationID = LocationUtils::cityToLocationID(localCityID);
	return LocationUtils::getClimateType(locationID, provinceID, binaryAssetLibrary);
}

ClimateType LocationUtils::getDungeonClimateType(int localDungeonID, int provinceID,
	const BinaryAssetLibrary &binaryAssetLibrary)
{
	const int locationID = LocationUtils::dungeonToLocationID(localDungeonID);
	return LocationUtils::getClimateType(locationID, provinceID, binaryAssetLibrary);
}

std::string LocationUtils::getMainQuestDungeonMifName(uint32_t dungeonSeed)
{
	// @todo: robustness - should this use any padding?
	const std::string seedString = std::to_string(dungeonSeed);
	const std::string mifName = seedString.substr(0, 8) + ".MIF";
	return mifName;
}

Int2 LocationUtils::getGlobalPoint(const Int2 &localPoint, const Rect &provinceRect)
{
	const int globalX = ((localPoint.x * ((provinceRect.getWidth() * 100) / 320)) / 100) + provinceRect.getLeft();
	const int globalY = ((localPoint.y * ((provinceRect.getHeight() * 100) / 200)) / 100) + provinceRect.getTop();
	return Int2(globalX, globalY);
}

Int2 LocationUtils::getLocalPoint(const Int2 &globalPoint, const Rect &provinceRect)
{
	const int localX = ((globalPoint.x - provinceRect.getLeft()) * 100) / ((provinceRect.getWidth() * 100) / 320);
	const int localY = ((globalPoint.y - provinceRect.getTop()) * 100) / ((provinceRect.getHeight() * 100) / 200);
	return Int2(localX, localY);
}

Int2 LocationUtils::getLocalCityPoint(uint32_t citySeed)
{
	return Int2(citySeed >> 16, citySeed & 0xFFFF);
}

int LocationUtils::getGlobalQuarter(const Int2 &globalPoint, const CityDataFile &cityData)
{
	// Find the province that contains the global point.
	int provinceID = -1;
	Rect provinceRect;
	for (int i = 0; i < CityDataFile::PROVINCE_COUNT; i++)
	{
		const CityDataFile::ProvinceData &province = cityData.getProvinceData(i);
		const Rect &curProvinceRect = province.getGlobalRect();

		if (curProvinceRect.containsInclusive(globalPoint))
		{
			provinceID = i;
			provinceRect = curProvinceRect;
			break;
		}
	}

	DebugAssertMsg(provinceID != -1, "No matching province for global point (" +
		std::to_string(globalPoint.x) + ", " + std::to_string(globalPoint.y) + ").");

	const Int2 localPoint = LocationUtils::getLocalPoint(globalPoint, provinceRect);

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

double LocationUtils::getLatitude(const Int2 &globalPoint)
{
	return (100.0 - static_cast<double>(globalPoint.y)) / 100.0;
}

int LocationUtils::getMapDistance(const Int2 &globalSrc, const Int2 &globalDst)
{
	const int dx = std::abs(globalSrc.x - globalDst.x);
	const int dy = std::abs(globalSrc.y - globalDst.y);
	return std::max(dx, dy) + (std::min(dx, dy) / 4);
}

int LocationUtils::getTravelDays(const Int2 &startGlobalPoint, const Int2 &endGlobalPoint,
	int month, const std::array<WeatherType, 36> &weathers, ArenaRandom &random,
	const BinaryAssetLibrary &binaryAssetLibrary)
{
	const auto &cityData = binaryAssetLibrary.getCityDataFile();

	// Get all the points along the line between the two points.
	const std::vector<Int2> points = MathUtils::bresenhamLine(startGlobalPoint, endGlobalPoint);

	int totalTime = 0;
	for (const Int2 &point : points)
	{
		const int monthIndex = (month + (totalTime / 3000)) % 12;
		const int weatherIndex = [&weathers, &cityData, &point]()
		{
			// Find which province quarter the global point is in.
			const int quarterIndex = LocationUtils::getGlobalQuarter(point, cityData);

			// Convert the weather type to its equivalent index.
			DebugAssertIndex(weathers, quarterIndex);
			return static_cast<int>(weathers[quarterIndex]);
		}();

		// The type of terrain at the world map point.
		const auto &worldMapTerrain = binaryAssetLibrary.getWorldMapTerrain();
		const uint8_t terrainIndex = BinaryAssetLibrary::WorldMapTerrain::getNormalizedIndex(
			worldMapTerrain.getAt(point.x, point.y));

		// Calculate the travel speed based on climate and weather.
		const auto &exeData = binaryAssetLibrary.getExeData();
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

uint32_t LocationUtils::getCitySeed(int localCityID, const CityDataFile::ProvinceData &province)
{
	const int locationID = LocationUtils::cityToLocationID(localCityID);
	const auto &location = province.getLocationData(locationID);
	return static_cast<uint32_t>((location.x << 16) + location.y);
}

uint32_t LocationUtils::getWildernessSeed(int localCityID, const CityDataFile::ProvinceData &province)
{
	const auto &location = province.getLocationData(LocationUtils::cityToLocationID(localCityID));
	const std::string &locationName = location.name;
	if (locationName.size() < 4)
	{
		// Can't generate seed -- return 0 for now. Can change later if there are short names in mods.
		return 0;
	}

	// Use the first four letters as the seed.
	const uint8_t *ptr = reinterpret_cast<const uint8_t*>(locationName.data());
	return Bytes::getLE32(ptr);
}

uint32_t LocationUtils::getRulerSeed(const Int2 &localPoint, const Rect &provinceRect)
{
	const Int2 globalPoint = LocationUtils::getGlobalPoint(localPoint, provinceRect);
	const uint32_t seed = static_cast<uint32_t>((globalPoint.x << 16) + globalPoint.y);
	return Bytes::rol(seed, 16);
}

uint32_t LocationUtils::getDistantSkySeed(const Int2 &localPoint, int provinceID,
	const Rect &provinceRect)
{
	const Int2 globalPoint = LocationUtils::getGlobalPoint(localPoint, provinceRect);
	const uint32_t seed = static_cast<uint32_t>((globalPoint.x << 16) + globalPoint.y);
	return seed * provinceID;
}

uint32_t LocationUtils::getDungeonSeed(int localDungeonID, int provinceID,
	const CityDataFile::ProvinceData &province)
{
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

uint32_t LocationUtils::getProvinceSeed(int provinceID, const CityDataFile::ProvinceData &province)
{
	const uint32_t provinceSeed = ((province.globalX << 16) + province.globalY) * provinceID;
	return provinceSeed;
}

uint32_t LocationUtils::getWildernessDungeonSeed(int provinceID,
	const CityDataFile::ProvinceData &province, int wildBlockX, int wildBlockY)
{
	const uint32_t provinceSeed = LocationUtils::getProvinceSeed(provinceID, province);
	return (provinceSeed + (((wildBlockY << 6) + wildBlockX) & 0xFFFF)) & 0xFFFFFFFF;
}

bool LocationUtils::isRulerMale(int localCityID, const CityDataFile::ProvinceData &province)
{
	const auto &location = province.getLocationData(localCityID);
	const Int2 localPoint(location.x, location.y);
	const uint32_t rulerSeed = LocationUtils::getRulerSeed(localPoint, province.getGlobalRect());
	return (rulerSeed & 0x3) != 0;
}

int LocationUtils::getCityTemplateCount(bool isCoastal, bool isCityState)
{
	return isCoastal ? (isCityState ? 3 : 2) : 5;
}

int LocationUtils::getCityTemplateNameIndex(LocationType locationType, bool isCoastal)
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
		DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(locationType)));
	}
}

int LocationUtils::getCityStartingPositionIndex(LocationType locationType,
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
		DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(locationType)));
	}
}

int LocationUtils::getCityReservedBlockListIndex(bool isCoastal, int templateID)
{
	return isCoastal ? (5 + templateID) : templateID;
}
