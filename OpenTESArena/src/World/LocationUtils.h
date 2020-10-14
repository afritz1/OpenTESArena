#ifndef LOCATION_UTILS_H
#define LOCATION_UTILS_H

#include <array>
#include <cstdint>
#include <string>

#include "../Assets/CityDataFile.h"
#include "../Math/Vector2.h"

class ArenaRandom;
class BinaryAssetLibrary;
class Rect;

enum class ClimateType;
enum class LocationType;
enum class WeatherType;

// Various functions for working with original game values like location IDs.

namespace LocationUtils
{
	// Required for handling the original game's special case with the center province's
	// premade city.
	constexpr int CENTER_PROVINCE_ID = 8;

	// Width and height of wild dungeons in chunks.
	constexpr int WILD_DUNGEON_WIDTH_CHUNK_COUNT = 2;
	constexpr int WILD_DUNGEON_HEIGHT_CHUNK_COUNT = 2;

	// Converts the given ID to a location ID (0..47). Location IDs are used with certain
	// calculations such as travel time, and must be "local" (i.e., never mixed with a province ID).
	int cityToLocationID(int localCityID);
	int dungeonToLocationID(int localDungeonID);

	// Converts a local city ID + province ID pair to a global city ID.
	int getGlobalCityID(int localCityID, int provinceID);

	// Converts a global city ID to a local city ID + province ID pair.
	std::pair<int, int> getLocalCityAndProvinceID(int globalCityID);

	// Functions for obtaining the local location type from a local city/dungeon ID.
	LocationType getCityType(int localCityID);
	LocationType getDungeonType(int localDungeonID);

	// Functions for getting the climate type of a location.
	ClimateType getCityClimateType(int localCityID, int provinceID,
		const BinaryAssetLibrary &binaryAssetLibrary);
	ClimateType getDungeonClimateType(int localDungeonID, int provinceID,
		const BinaryAssetLibrary &binaryAssetLibrary);

	// Gets the .MIF name for a main quest dungeon, given its seed from getDungeonSeed().
	std::string getMainQuestDungeonMifName(uint32_t dungeonSeed);

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

	// Gets the approximation of the linear distance between two global points. This value is
	// used to display the distance in kilometers.
	int getMapDistance(const Int2 &globalSrc, const Int2 &globalDst);

	// Gets the number of days required to travel from one province's local point to another.
	int getTravelDays(const Int2 &startGlobalPoint, const Int2 &endGlobalPoint,
		int month, const std::array<WeatherType, 36> &weathers, ArenaRandom &random,
		const BinaryAssetLibrary &binaryAssetLibrary);

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

	// Gets the number of .MIF templates to choose from for a city.
	int getCityTemplateCount(bool isCoastal, bool isCityState);

	// Gets an index into the template name array (town%d.mif, ..., cityw%d.mif).
	int getCityTemplateNameIndex(LocationType locationType, bool isCoastal);

	// Gets an index into the city starting positions list. This determines how city blocks
	// are offset within the city skeleton.
	int getCityStartingPositionIndex(LocationType locationType, bool isCoastal, int templateID);

	// Gets an index into the city reserved block list.
	int getCityReservedBlockListIndex(bool isCoastal, int templateID);
}

#endif
