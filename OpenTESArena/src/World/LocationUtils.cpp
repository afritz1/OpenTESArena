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

double LocationUtils::getLatitude(const Location &location, const CityDataFile &cityData)
{
	const Int2 globalPoint = [&location, &cityData]()
	{
		const int locationID = [&location]()
		{
			if (location.dataType == LocationDataType::City)
			{
				return LocationUtils::cityToLocationID(location.localCityID);
			}
			else if (location.dataType == LocationDataType::Dungeon)
			{
				return LocationUtils::dungeonToLocationID(location.localDungeonID);
			}
			else if (location.dataType == LocationDataType::SpecialCase)
			{
				if (location.specialCaseType == Location::SpecialCaseType::StartDungeon)
				{
					// Get location ID of the center city.
					return 0;
				}
				else if (location.specialCaseType == Location::SpecialCaseType::WildDungeon)
				{
					// Return the point of the city the wild dungeon is near.
					return LocationUtils::cityToLocationID(location.localCityID);
				}
				else
				{
					DebugUnhandledReturnMsg(int,
						std::to_string(static_cast<int>(location.specialCaseType)));
				}
			}
			else
			{
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(location.dataType)));
			}
		}();

		const auto &province = cityData.getProvinceData(location.provinceID);
		const Int2 localPoint = [&province, locationID]()
		{
			const auto &location = province.getLocationData(locationID);
			return Int2(location.x, location.y);
		}();

		return CityDataFile::localPointToGlobal(localPoint, province.getGlobalRect());
	}();

	return (100.0 - static_cast<double>(globalPoint.y)) / 100.0;
}
