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
