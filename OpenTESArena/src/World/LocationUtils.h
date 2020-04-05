#ifndef LOCATION_UTILS_H
#define LOCATION_UTILS_H

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
}

#endif
