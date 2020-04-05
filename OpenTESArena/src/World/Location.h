#ifndef LOCATION_H
#define LOCATION_H

#include <string>

// A location is any place on a province map. It is either a city or a dungeon, with
// different varieties of each.

class CityDataFile;
class ExeData;
class MiscAssets;

enum class ClimateType;
enum class LocationDataType;
enum class LocationType;

class Location
{
public:
	enum class SpecialCaseType
	{
		StartDungeon,
		WildDungeon // Only for testing (in reality a wild dungeon doesn't affect the location).
	};

	// Center province on the world map, excluded from certain groups and calculations.
	static const int CENTER_PROVINCE_ID;

	LocationDataType dataType; // Determines how the union is accessed.

	union
	{
		int localCityID; // 0..31.
		int localDungeonID; // 0..15.
		SpecialCaseType specialCaseType;
	};

	int provinceID;

	// Methods for constructing a certain type of location.
	static Location makeCity(int localCityID, int provinceID);
	static Location makeDungeon(int localDungeonID, int provinceID);
	static Location makeSpecialCase(Location::SpecialCaseType specialCaseType, int provinceID);
	static Location makeFromLocationID(int locationID, int provinceID);

	// Functions for obtaining the local location type from a local city/dungeon ID.
	static LocationType getCityType(int localCityID);
	static LocationType getDungeonType(int localDungeonID);

	// Gets the display name of a location. This is the name shown in places like province maps
	// and the status pop-up. Some locations (like named/wild dungeons) do not show their name
	// on the automap.
	const std::string &getName(const CityDataFile &cityData, const ExeData &exeData) const;

	// Gets the latitude of a location in normalized [-1.0, 1.0] Arena angle units. 0 at the
	// equator, 1.0 at the north pole, and -1.0 at the south pole.
	double getLatitude(const CityDataFile &cityData) const;
};

#endif
