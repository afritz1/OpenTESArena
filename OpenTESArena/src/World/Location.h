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
};

#endif
