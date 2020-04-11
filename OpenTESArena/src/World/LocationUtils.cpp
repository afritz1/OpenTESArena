#include "Location.h"
#include "LocationDataType.h"
#include "LocationUtils.h"
#include "../Assets/CityDataFile.h"
#include "../Assets/MiscAssets.h"
#include "../Math/Vector2.h"

#include "components/debug/Debug.h"

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
