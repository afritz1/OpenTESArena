#include "Location.h"
#include "LocationDataType.h"
#include "LocationUtils.h"
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
		const Int2 globalPoint = CityDataFile::localPointToGlobal(
			localPoint, province.getGlobalRect());
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
	return CityDataFile::localPointToGlobal(localPoint, provinceRect);
}

double LocationUtils::getLatitude(const Int2 &globalPoint)
{
	return (100.0 - static_cast<double>(globalPoint.y)) / 100.0;
}
