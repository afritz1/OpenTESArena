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

	static constexpr int PROVINCE_COUNT = 9;
private:
	// These are ordered the same as usual (read left to right, and center is last).
	std::array<ProvinceData, PROVINCE_COUNT> provinces;
public:
	// Gets the province data at the given province index.
	CityDataFile::ProvinceData &getProvinceData(int index);
	const CityDataFile::ProvinceData &getProvinceData(int index) const;

	// @todo: this should be in some LevelUtils namespace.
	// Gets the offset value of a door voxel in the world. Used with various calculations
	// (.MIF name, lock level).
	static uint16_t getDoorVoxelOffset(int x, int y);

	// @todo: this should be in some LevelUtils namespace.
	// Gets the .MIF name for a door voxel in a city or the wilderness.
	std::string getDoorVoxelMifName(int x, int y, int menuID, int localCityID, int provinceID,
		bool isCity, const ExeData &exeData) const;

	// @todo: this should be in some LevelUtils namespace.
	// Gets the lock level for a door voxel at the given XY coordinate.
	static int getDoorVoxelLockLevel(int x, int y, ArenaRandom &random);

	// @todo: this should be in some LevelUtils namespace.
	// Gets the '#' number used in IN#.0x and RE#.0x save files.
	static int getServiceSaveFileNumber(int doorX, int doorY);
	static int getWildernessServiceSaveFileNumber(int wildX, int wildY);

	bool init(const char *filename);
};

#endif
