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
class ExeData;
class MiscAssets;

enum class LocationType;
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
			std::string name; // Up to twenty chars, null-terminated.
			uint16_t x, y; // Position on screen.
			uint8_t visibility; // Visibility on map. Only used for dungeons. 0x02 = visible.

			bool isVisible() const;
			void setVisible(bool visible);
		};

		std::string name; // Up to twenty chars, null-terminated.
		uint16_t globalX, globalY, globalW, globalH; // Province-to-world-map projection.
		std::array<LocationData, 8> cityStates;
		std::array<LocationData, 8> towns;
		std::array<LocationData, 16> villages;
		LocationData secondDungeon; // Staff dungeon.
		LocationData firstDungeon; // Staff map dungeon.
		std::array<LocationData, 14> randomDungeons; // Random names, fixed locations.

		// Creates a rectangle from the province's global {X,Y,W,H} values.
		Rect getGlobalRect() const;

		// Gets the location associated with the given location ID.
		const CityDataFile::ProvinceData::LocationData &getLocationData(int locationID) const;
	};
private:
	// These are ordered the same as usual (read left to right, and center is last).
	std::array<ProvinceData, 9> provinces;
public:
	static const int PROVINCE_COUNT;

	// Gets the province data at the given province index.
	CityDataFile::ProvinceData &getProvinceData(int index);
	const CityDataFile::ProvinceData &getProvinceData(int index) const;

	// Converts a local city ID + province ID pair to a global city ID.
	static int getGlobalCityID(int localCityID, int provinceID);

	// Converts a global city ID to a local city ID + province ID pair.
	static std::pair<int, int> getLocalCityAndProvinceID(int globalCityID);

	// Gets the approximation of the linear distance between two global points.
	static int getDistance(const Int2 &globalSrc, const Int2 &globalDst);

	// Converts a local XY coordinate to global coordinates. The rectangle comes from
	// the province header.
	static Int2 localPointToGlobal(const Int2 &localPoint, const Rect &rect);

	// Converts a global XY coordinate to local coordinates. The rectangle comes from
	// the province header.
	static Int2 globalPointToLocal(const Int2 &globalPoint, const Rect &rect);

	// Gets the .MIF name for a main quest dungeon, given its seed from getDungeonSeed().
	static std::string getMainQuestDungeonMifName(uint32_t seed);

	// Gets the offset value of a door voxel in the world. Used with various calculations
	// (.MIF name, lock level).
	static uint16_t getDoorVoxelOffset(int x, int y);

	// Gets the .MIF name for a door voxel in a city or the wilderness.
	std::string getDoorVoxelMifName(int x, int y, int menuID, int localCityID, int provinceID,
		bool isCity, const ExeData &exeData) const;

	// Gets the lock level for a door voxel at the given XY coordinate.
	static int getDoorVoxelLockLevel(int x, int y, ArenaRandom &random);

	// Gets the side length of a city in city blocks.
	static int getCityDimensions(LocationType locationType);

	// Gets the number of .MIF templates to choose from for a city.
	static int getCityTemplateCount(bool isCoastal, bool isCityState);

	// Gets an index into the template name array (town%d.mif, ..., cityw%d.mif).
	static int getCityTemplateNameIndex(LocationType locationType, bool isCoastal);

	// Gets an index into the city starting positions list. This determines how city blocks
	// are offset within the city skeleton.
	static int getCityStartingPositionIndex(LocationType locationType,
		bool isCoastal, int templateID);

	// Gets an index into the city reserved block list.
	static int getCityReservedBlockListIndex(bool isCoastal, int templateID);

	// Gets the '#' number used in IN#.0x and RE#.0x save files.
	static int getServiceSaveFileNumber(int doorX, int doorY);
	static int getWildernessServiceSaveFileNumber(int wildX, int wildY);

	// Gets the quarter within a province (to determine weather).
	int getGlobalQuarter(const Int2 &globalPoint) const;

	// Gets the number of days required to travel from one location to another.
	int getTravelDays(int startLocationID, int startProvinceID, int endLocationID,
		int endProvinceID, int month, const std::array<WeatherType, 36> &weathers,
		ArenaRandom &random, const MiscAssets &miscAssets) const;

	// Gets the 32-bit seed for a city.
	uint32_t getCitySeed(int localCityID, int provinceID) const;

	// Gets the 32-bit seed for a city's wilderness.
	uint32_t getWildernessSeed(int localCityID, int provinceID) const;

	// Gets the 32-bit seed for a city's ruler.
	uint32_t getRulerSeed(int localCityID, int provinceID) const;

	// Gets the 32-bit seed for a city's distant sky.
	uint32_t getDistantSkySeed(int localCityID, int provinceID) const;

	// Gets the 32-bit seed for a dungeon, given a dungeon ID and province ID, where
	// the dungeon ID is between 0 and 15.
	uint32_t getDungeonSeed(int localDungeonID, int provinceID) const;

	// Gets the 32-bit seed for a wilderness dungeon, given a province ID and X and Y
	// wilderness block coordinates.
	uint32_t getWildernessDungeonSeed(int provinceID, int wildBlockX, int wildBlockY) const;

	// Gets the global X and Y coordinates of a local city in the given province.
	Int2 getGlobalPoint(int localCityID, int provinceID) const;

	// Gets the local X and Y coordinates for a city from its seed.
	static Int2 getLocalCityPoint(uint32_t citySeed);

	void init(const std::string &filename);
};

#endif
