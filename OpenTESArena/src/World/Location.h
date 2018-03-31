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
private:
	// Parent function for getting the climate type of a location.
	static ClimateType getClimateType(int locationID, int provinceID,
		const MiscAssets &miscAssets);
public:
	enum class SpecialCaseType
	{
		StartDungeon,
		WildDungeon // Only for testing (in reality a wild dungeon doesn't affect the location).
	};

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

	// Functions for getting the climate type of a location.
	static ClimateType getCityClimateType(int localCityID, int provinceID,
		const MiscAssets &miscAssets);
	static ClimateType getDungeonClimateType(int localDungeonID, int provinceID,
		const MiscAssets &miscAssets);

	// Converts the given ID to a location ID (0..47). Location IDs are used with certain
	// calculations such as travel time, and must be "local" (i.e., never mixed with a
	// province ID).
	static int cityToLocationID(int localCityID);
	static int dungeonToLocationID(int localDungeonID);

	// Gets the display name of a location. This is the name shown in places like province maps
	// and the status pop-up. Some locations (like named/wild dungeons) do not show their name
	// on the automap.
	const std::string &getName(const CityDataFile &cityData, const ExeData &exeData) const;
};

#endif
