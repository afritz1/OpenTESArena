#ifndef LOCATION_UTILS_H
#define LOCATION_UTILS_H

#include "../Math/Vector2.h"

class CityDataFile;
class Location;
class MiscAssets;
class Rect;

enum class ClimateType;

// Various functions for working with original game values like location IDs.

namespace LocationUtils
{
	// Converts the given ID to a location ID (0..47). Location IDs are used with certain
	// calculations such as travel time, and must be "local" (i.e., never mixed with a province ID).
	int cityToLocationID(int localCityID);
	int dungeonToLocationID(int localDungeonID);

	// Converts a local city ID + province ID pair to a global city ID.
	int getGlobalCityID(int localCityID, int provinceID);

	// Converts a global city ID to a local city ID + province ID pair.
	std::pair<int, int> getLocalCityAndProvinceID(int globalCityID);

	// Functions for getting the climate type of a location.
	ClimateType getCityClimateType(int localCityID, int provinceID, const MiscAssets &miscAssets);
	ClimateType getDungeonClimateType(int localDungeonID, int provinceID, const MiscAssets &miscAssets);

	// Converts a location point in a province map to the equivalent world map point.
	Int2 getGlobalPoint(const Int2 &localPoint, const Rect &provinceRect);

	// Converts a global XY coordinate to local coordinates. The rectangle comes from
	// the province header.
	Int2 getLocalPoint(const Int2 &globalPoint, const Rect &provinceRect);

	// Gets the quarter within a province (to determine weather).
	int getGlobalQuarter(const Int2 &globalPoint, const CityDataFile &cityData);

	// Gets the latitude of a location on the world map in normalized [-1.0, 1.0] Arena angle units.
	// 0 at the equator, 1.0 at the north pole, and -1.0 at the south pole.
	double getLatitude(const Int2 &globalPoint);
}

#endif
