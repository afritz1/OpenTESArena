#include "ClimateType.h"
#include "Location.h"
#include "LocationDataType.h"
#include "LocationType.h"
#include "../Assets/CityDataFile.h"
#include "../Assets/ExeData.h"
#include "../Assets/MiscAssets.h"
#include "../Utilities/Debug.h"

const int Location::CENTER_PROVINCE_ID = 8;

Location Location::makeCity(int localCityID, int provinceID)
{
	Location location;
	location.dataType = LocationDataType::City;
	location.localCityID = localCityID;
	location.provinceID = provinceID;
	return location;
}

Location Location::makeDungeon(int localDungeonID, int provinceID)
{
	Location location;
	location.dataType = LocationDataType::Dungeon;
	location.localDungeonID = localDungeonID;
	location.provinceID = provinceID;
	return location;
}

Location Location::makeSpecialCase(Location::SpecialCaseType specialCaseType, int provinceID)
{
	Location location;
	location.dataType = LocationDataType::SpecialCase;
	location.specialCaseType = specialCaseType;
	location.provinceID = provinceID;
	return location;
}

Location Location::makeFromLocationID(int locationID, int provinceID)
{
	if (locationID < 32)
	{
		return Location::makeCity(locationID, provinceID);
	}
	else if (locationID < 48)
	{
		return Location::makeDungeon(locationID - 32, provinceID);
	}
	else
	{
		throw DebugException("Bad location ID \"" + std::to_string(locationID) + "\".");
	}
}

LocationType Location::getCityType(int localCityID)
{
	if (localCityID < 8)
	{
		return LocationType::CityState;
	}
	else if (localCityID < 16)
	{
		return LocationType::Town;
	}
	else
	{
		return LocationType::Village;
	}
}

LocationType Location::getDungeonType(int localDungeonID)
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

ClimateType Location::getClimateType(int locationID, int provinceID,
	const MiscAssets &miscAssets)
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

ClimateType Location::getCityClimateType(int localCityID, int provinceID,
	const MiscAssets &miscAssets)
{
	const int locationID = Location::cityToLocationID(localCityID);
	return Location::getClimateType(locationID, provinceID, miscAssets);
}

ClimateType Location::getDungeonClimateType(int localDungeonID, int provinceID,
	const MiscAssets &miscAssets)
{
	const int locationID = Location::dungeonToLocationID(localDungeonID);
	return Location::getClimateType(locationID, provinceID, miscAssets);
}

int Location::cityToLocationID(int localCityID)
{
	return localCityID;
}

int Location::dungeonToLocationID(int localDungeonID)
{
	return localDungeonID + 32;
}

const std::string &Location::getName(const CityDataFile &cityData, const ExeData &exeData) const
{
	const auto &province = cityData.getProvinceData(this->provinceID);

	if (this->dataType == LocationDataType::City)
	{
		const int locationID = Location::cityToLocationID(this->localCityID);
		const auto &locationData = province.getLocationData(locationID);
		return locationData.name;
	}
	else if (this->dataType == LocationDataType::Dungeon)
	{
		const int locationID = Location::dungeonToLocationID(this->localDungeonID);
		const auto &locationData = province.getLocationData(locationID);
		return locationData.name;
	}
	else if (this->dataType == LocationDataType::SpecialCase)
	{
		if (this->specialCaseType == Location::SpecialCaseType::StartDungeon)
		{
			return exeData.locations.startDungeonName;
		}
		else if (this->specialCaseType == Location::SpecialCaseType::WildDungeon)
		{
			// Return the name of the city the wild dungeon is near.
			const int locationID = Location::cityToLocationID(this->localCityID);
			const auto &locationData = province.getLocationData(locationID);
			return locationData.name;
		}
		else
		{
			throw DebugException("Bad special case type \"" +
				std::to_string(static_cast<int>(this->specialCaseType)) + "\".");
		}
	}
	else
	{
		throw DebugException("Bad location data type \"" +
			std::to_string(static_cast<int>(this->dataType)) + "\".");
	}
}

double Location::getLatitude(const CityDataFile &cityData) const
{
	const Int2 globalPoint = [this, &cityData]()
	{
		const int locationID = [this]()
		{
			if (this->dataType == LocationDataType::City)
			{
				return Location::cityToLocationID(this->localCityID);
			}
			else if (this->dataType == LocationDataType::Dungeon)
			{
				return Location::dungeonToLocationID(this->localDungeonID);
			}
			else if (this->dataType == LocationDataType::SpecialCase)
			{
				if (this->specialCaseType == Location::SpecialCaseType::StartDungeon)
				{
					// Get location ID of the center city.
					return 0;
				}
				else if (this->specialCaseType == Location::SpecialCaseType::WildDungeon)
				{
					// Return the point of the city the wild dungeon is near.
					return Location::cityToLocationID(this->localCityID);
				}
				else
				{
					throw DebugException("Bad special case type \"" +
						std::to_string(static_cast<int>(this->specialCaseType)) + "\".");
				}
			}
			else
			{
				throw DebugException("Bad location data type \"" +
					std::to_string(static_cast<int>(this->dataType)) + "\".");
			}
		}();

		const auto &province = cityData.getProvinceData(this->provinceID);
		const Int2 localPoint = [&province, locationID]()
		{
			const auto &location = province.getLocationData(locationID);
			return Int2(location.x, location.y);
		}();

		return CityDataFile::localPointToGlobal(localPoint, province.getGlobalRect());
	}();

	return (100.0 - static_cast<double>(globalPoint.y)) / 100.0;
}
