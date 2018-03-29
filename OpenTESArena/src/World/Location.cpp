#include "ClimateType.h"
#include "Location.h"
#include "LocationDataType.h"
#include "LocationType.h"
#include "../Assets/ExeData.h"
#include "../Assets/MiscAssets.h"

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
		throw std::runtime_error("Bad location ID \"" + std::to_string(locationID) + "\".");
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
	const Int2 globalPoint = cityData.localPointToGlobal(localPoint, province.getGlobalRect());
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

std::string Location::getName(const MiscAssets &miscAssets) const
{
	const auto &cityData = miscAssets.getCityDataFile();
	const auto &province = cityData.getProvinceData(this->provinceID);

	if (this->dataType == LocationDataType::City)
	{
		const int locationID = Location::cityToLocationID(this->localCityID);
		const auto &locationData = province.getLocationData(locationID);
		return std::string(locationData.name.data());
	}
	else if (this->dataType == LocationDataType::Dungeon)
	{
		const int locationID = Location::dungeonToLocationID(this->localDungeonID);
		const auto &locationData = province.getLocationData(locationID);
		return std::string(locationData.name.data());
	}
	else if (this->dataType == LocationDataType::SpecialCase)
	{
		if (this->specialCaseType == Location::SpecialCaseType::StartDungeon)
		{
			const auto &exeData = miscAssets.getExeData();
			return exeData.locations.startDungeonName;
		}
		else if (this->specialCaseType == Location::SpecialCaseType::WildDungeon)
		{
			// Return the name of the city the wild dungeon is near.
			const int locationID = Location::cityToLocationID(this->localCityID);
			const auto &locationData = province.getLocationData(locationID);
			return std::string(locationData.name.data());
		}
		else
		{
			throw std::runtime_error("Bad special case type \"" +
				std::to_string(static_cast<int>(this->specialCaseType)) + "\".");
		}
	}
	else
	{
		throw std::runtime_error("Bad location data type \"" +
			std::to_string(static_cast<int>(this->dataType)) + "\".");
	}
}
