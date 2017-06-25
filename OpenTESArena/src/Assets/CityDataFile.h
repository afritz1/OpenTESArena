#ifndef CITY_DATA_FILE_H
#define CITY_DATA_FILE_H

#include <array>
#include <cstdint>
#include <string>

// CITYDATA.00 contains basic data for locations in each province on the world map.
// It has the names of each place and their XY coordinates on the screen.

// CITYDATA.64 and CITYDATA.65 are most likely duplicates of CITYDATA.00.

class CityDataFile
{
public:
	// Each province contains 8 city-states, 8 towns, 16 villages, and two main quest dungeons.
	// The center province is an exception; it has just 1 city (all others are zeroed out).
	struct ProvinceData
	{
		struct LocationData
		{
			std::array<char, 20> name; // Null-terminated.
			uint16_t x, y; // Position on screen.
		};

		// There is some unknown data, and it's 384 bytes per province, which would appear
		// to be 12 bytes per location. Perhaps they are seed values, or maybe coordinates for
		// random dungeons? Climate IDs (grassy, desert, cold, snowy)?
		struct UnknownData
		{
			std::array<char, 384> data;
		};

		std::array<char, 20> name; // Null-terminated.
		std::array<char, 8> unknown; // Province-relevant data...?
		std::array<LocationData, 8> cityStates;
		std::array<LocationData, 8> towns;
		std::array<LocationData, 16> villages;
		LocationData secondDungeon;
		LocationData firstDungeon;
		UnknownData unknownData;
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
