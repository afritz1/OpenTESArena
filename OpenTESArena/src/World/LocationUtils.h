#ifndef LOCATION_UTILS_H
#define LOCATION_UTILS_H

class Location;
class MiscAssets;

enum class ClimateType;

// Various functions for working with original game values like location IDs.

namespace LocationUtils
{
	// Converts the given ID to a location ID (0..47). Location IDs are used with certain
	// calculations such as travel time, and must be "local" (i.e., never mixed with a province ID).
	int cityToLocationID(int localCityID);
	int dungeonToLocationID(int localDungeonID);

	// Functions for getting the climate type of a location.
	ClimateType getCityClimateType(int localCityID, int provinceID, const MiscAssets &miscAssets);
	ClimateType getDungeonClimateType(int localDungeonID, int provinceID, const MiscAssets &miscAssets);

	// Gets the latitude of a location in normalized [-1.0, 1.0] Arena angle units. 0 at the
	// equator, 1.0 at the north pole, and -1.0 at the south pole.
	double getLatitude(const Location &location, const CityDataFile &cityData);
}

#endif
