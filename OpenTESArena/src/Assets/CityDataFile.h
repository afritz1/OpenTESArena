#ifndef CITY_DATA_FILE_H
#define CITY_DATA_FILE_H

#include <array>
#include <cstdint>
#include <string>

// CITYDATA.00 contains basic data for locations in each province on the world map.
// It has the names of each place and their XY coordinates on the screen.

// *.64 files are for swapping, *.65 files are templates for new characters, and
// *.0x files contain save-specific modifications (i.e., to save random dungeon names).

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
			std::array<char, 20> name; // Null-terminated.
			uint16_t x, y; // Position on screen.
			uint8_t visibility; // Visibility on map. Only used for dungeons. 0x02 = visible.
		};

		std::array<char, 20> name; // Null-terminated.
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
	CityDataFile(const std::string &filename);
	~CityDataFile();

	static const int PROVINCE_COUNT;

	// Gets the province data at the given province index.
	const CityDataFile::ProvinceData &getProvinceData(int index) const;
};

#endif
