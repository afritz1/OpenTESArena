#include "Location.h"
#include "LocationDataType.h"
#include "LocationUtils.h"
#include "../Assets/MiscAssets.h"
#include "../Math/Vector2.h"

#include "components/debug/Debug.h"
#include "components/utilities/Bytes.h"

namespace LocationUtils
{
	// Parent function for getting the climate type of a location.
	ClimateType getClimateType(int locationID, int provinceID, const MiscAssets &miscAssets)
	{
		const auto &cityData = miscAssets.getCityDataFile();
		const auto &province = cityData.getProvinceData(provinceID);
		const auto &location = province.getLocationData(locationID);
		const Int2 localPoint(location.x, location.y);
		const Int2 globalPoint = LocationUtils::getGlobalPoint(localPoint, province.getGlobalRect());
		const auto &worldMapTerrain = miscAssets.getWorldMapTerrain();
		const uint8_t terrain = worldMapTerrain.getFailSafeAt(globalPoint.x, globalPoint.y);
		return MiscAssets::WorldMapTerrain::toClimateType(terrain);
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

ClimateType LocationUtils::getCityClimateType(int localCityID, int provinceID,
	const MiscAssets &miscAssets)
{
	const int locationID = LocationUtils::cityToLocationID(localCityID);
	return LocationUtils::getClimateType(locationID, provinceID, miscAssets);
}

ClimateType LocationUtils::getDungeonClimateType(int localDungeonID, int provinceID,
	const MiscAssets &miscAssets)
{
	const int locationID = LocationUtils::dungeonToLocationID(localDungeonID);
	return LocationUtils::getClimateType(locationID, provinceID, miscAssets);
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
