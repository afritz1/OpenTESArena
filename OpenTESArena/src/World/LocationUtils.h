#ifndef LOCATION_UTILS_H
#define LOCATION_UTILS_H

#include <cstdint>

#include "../Assets/CityDataFile.h"
#include "../Math/Vector2.h"

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

	// Gets the local X and Y coordinates for a city from its seed.
	Int2 getLocalCityPoint(uint32_t citySeed);

	// Gets the quarter within a province (to determine weather).
	int getGlobalQuarter(const Int2 &globalPoint, const CityDataFile &cityData);

	// Gets the latitude of a location on the world map in normalized [-1.0, 1.0] Arena angle units.
	// 0 at the equator, 1.0 at the north pole, and -1.0 at the south pole.
	double getLatitude(const Int2 &globalPoint);

	// Gets the 32-bit seed for a city in the given province.
	uint32_t getCitySeed(int localCityID, const CityDataFile::ProvinceData &province);

	// Gets the 32-bit seed for a city's wilderness in the given province.
	uint32_t getWildernessSeed(int localCityID, const CityDataFile::ProvinceData &province);

	// Gets the 32-bit seed for a city's ruler in the given province's map. This doesn't
	// require actual location data -- it can just be a place on the map.
	uint32_t getRulerSeed(const Int2 &localPoint, const Rect &provinceRect);

	// Gets the 32-bit seed for a city's distant sky in the given province.
	uint32_t getDistantSkySeed(const Int2 &localPoint, int provinceID, const Rect &provinceRect);

	// Gets the 32-bit seed for a dungeon, given a dungeon ID and province ID.
	uint32_t getDungeonSeed(int localDungeonID, int provinceID,
		const CityDataFile::ProvinceData &province);

	// Gets the 32-bit seed for a province. Used with wilderness dungeons.
	uint32_t getProvinceSeed(int provinceID, const CityDataFile::ProvinceData &province);

	// Gets the 32-bit seed for a wilderness dungeon, given a province ID and X and Y
	// wilderness block coordinates.
	uint32_t getWildernessDungeonSeed(int provinceID,
		const CityDataFile::ProvinceData &province, int wildBlockX, int wildBlockY);

	// Gets whether the ruler of a city in the given province should be male.
	bool isRulerMale(int localCityID, const CityDataFile::ProvinceData &province);
}

#endif
