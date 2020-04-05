#include "ClimateType.h"
#include "Location.h"
#include "LocationDataType.h"
#include "LocationType.h"
#include "LocationUtils.h"
#include "../Assets/CityDataFile.h"
#include "../Assets/ExeData.h"
#include "../Assets/MiscAssets.h"

#include "components/debug/Debug.h"

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
		DebugUnhandledReturnMsg(Location, std::to_string(locationID));
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
	else if (localCityID < 32)
	{
		return LocationType::Village;
	}
	else
	{
		DebugUnhandledReturnMsg(LocationType, std::to_string(localCityID));
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

const std::string &Location::getName(const CityDataFile &cityData, const ExeData &exeData) const
{
	const auto &province = cityData.getProvinceData(this->provinceID);

	if (this->dataType == LocationDataType::City)
	{
		const int locationID = LocationUtils::cityToLocationID(this->localCityID);
		const auto &locationData = province.getLocationData(locationID);
		return locationData.name;
	}
	else if (this->dataType == LocationDataType::Dungeon)
	{
		const int locationID = LocationUtils::dungeonToLocationID(this->localDungeonID);
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
			const int locationID = LocationUtils::cityToLocationID(this->localCityID);
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
				return LocationUtils::cityToLocationID(this->localCityID);
			}
			else if (this->dataType == LocationDataType::Dungeon)
			{
				return LocationUtils::dungeonToLocationID(this->localDungeonID);
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
					return LocationUtils::cityToLocationID(this->localCityID);
				}
				else
				{
					DebugUnhandledReturnMsg(int,
						std::to_string(static_cast<int>(this->specialCaseType)));
				}
			}
			else
			{
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(this->dataType)));
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
