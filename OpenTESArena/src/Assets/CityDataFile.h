#ifndef CITY_DATA_FILE_H
#define CITY_DATA_FILE_H

#include <array>
#include <cstdint>
#include <string>

#include "../Math/Rect.h"
#include "../Math/Vector2.h"

// CITYDATA.00 contains basic data for locations in each province on the world map.
// It has the names of each place and their XY coordinates on the screen.

// *.64 files are for swapping, *.65 files are templates for new characters, and
// *.0x files contain save-specific modifications (i.e., to save random dungeon names).

class ArenaRandom;
class MiscAssets;

enum class WeatherType;

class CityDataFile
{
public:
	// Each province contains 8 city-states, 8 towns, 16 villages, 2 main quest dungeons,
	// and 14 spaces for random dungeons. The center province is an exception; it has just 1 
	// city (all others are zeroed out).
	struct ProvinceData
	{
		struct LocationData
		{
			std::array<char, 20> name; // Twenty chars, null-terminated.
			uint16_t x, y; // Position on screen.
			uint8_t visibility; // Visibility on map. Only used for dungeons. 0x02 = visible.
		};

		std::array<char, 20> name; // Twenty chars, null-terminated.
		uint16_t globalX, globalY, globalW, globalH; // Province-to-world-map projection.
		std::array<LocationData, 8> cityStates;
		std::array<LocationData, 8> towns;
		std::array<LocationData, 16> villages;
		LocationData secondDungeon;
		LocationData firstDungeon;
		std::array<LocationData, 14> randomDungeons; // Random names, fixed locations.
	};
private:
	// These are ordered the same as usual (read left to right, and center is last).
	std::array<ProvinceData, 9> provinces;
public:
	static const int PROVINCE_COUNT;

	// Converts a local city ID + province ID pair to a global city ID.
	static int getGlobalCityID(int localCityID, int provinceID);

	// Converts a global city ID to a local city ID + province ID pair.
	static std::pair<int, int> getLocalCityAndProvinceID(int globalCityID);

	// Gets the approximation of the linear distance between two points.
	static int getDistance(const Int2 &p1, const Int2 &p2);

	// Converts a local XY coordinate to global coordinates. The rectangle comes from
	// the province header.
	static Int2 localPointToGlobal(const Int2 &localPoint, const Rect &rect);

	// Converts a global XY coordinate to local coordinates. The rectangle comes from
	// the province header.
	static Int2 globalPointToLocal(const Int2 &globalPoint, const Rect &rect);

	// Gets the .MIF name for a main quest dungeon, given its seed from getDungeonSeed().
	static std::string getMainQuestDungeonMifName(uint32_t seed);

	// Gets the province data at the given province index.
	const CityDataFile::ProvinceData &getProvinceData(int index) const;

	// Gets the location associated with the given local location ID and province ID.
	const CityDataFile::ProvinceData::LocationData &getLocationData(
		int localLocationID, int provinceID) const;

	// Gets the quarter within a province (to determine weather).
	int getGlobalQuarter(const Int2 &globalPoint) const;

	// Gets the number of days required to travel from one location to another.
	int getTravelDays(int startLocalLocationID, int startProvinceID, int endLocalLocationID,
		int endProvinceID, int month, const std::array<WeatherType, 36> &weathers,
		ArenaRandom &random, const MiscAssets &miscAssets) const;

	// Gets the 32-bit seed for a dungeon, given a dungeon ID and province ID, where
	// the dungeon ID is between 0 and 15.
	uint32_t getDungeonSeed(int dungeonID, int provinceID) const;

	// Gets the 32-bit seed for a wilderness dungeon, given a province ID and X and Y
	// wilderness block coordinates.
	uint32_t getWildernessDungeonSeed(int provinceID, int wildBlockX, int wildBlockY) const;

	void init(const std::string &filename);
};

#endif
