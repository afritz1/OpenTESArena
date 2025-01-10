#ifndef CITY_DATA_FILE_H
#define CITY_DATA_FILE_H

#include <cstdint>
#include <string>

#include "../Math/Rect.h"

// One world map location.
struct ArenaLocationData
{
	std::string name; // Up to twenty chars, null-terminated.
	uint16_t x, y; // Position on screen.
	uint8_t visibility; // Visibility on map. Only used for dungeons. 0x02 = visible.

	bool isVisible() const;
	void setVisible(bool visible);
};

// Each province contains 8 city-states, 8 towns, 16 villages, 2 main quest dungeons,
// and 14 spaces for random dungeons. The center province is an exception; it has just 1 
// city (all others are zeroed out).
struct ArenaProvinceData
{
	std::string name; // Up to twenty chars, null-terminated.
	uint16_t globalX, globalY, globalW, globalH; // Province-to-world-map projection.
	ArenaLocationData cityStates[8];
	ArenaLocationData towns[8];
	ArenaLocationData villages[16];
	ArenaLocationData secondDungeon; // Staff dungeon.
	ArenaLocationData firstDungeon; // Staff map dungeon.
	ArenaLocationData randomDungeons[14]; // Random names, fixed locations.

	// Creates a rectangle from the province's global {X,Y,W,H} values.
	Rect getGlobalRect() const;

	// Gets the location associated with the given location ID.
	const ArenaLocationData &getLocationData(int locationID) const;
};

// CITYDATA.00 contains basic data for locations in each province on the world map.
// It has the names of each place and their XY coordinates on the screen. *.64 files are
// for swapping, *.65 files are templates for new characters, and *.0x files contain
// save-specific modifications (i.e., to save random dungeon names).
class CityDataFile
{
public:
	static constexpr int PROVINCE_COUNT = 9;
private:
	// These are ordered the same as usual (read left to right, and center is last).
	ArenaProvinceData provinces[PROVINCE_COUNT];
public:
	// Gets the province data at the given province index.
	ArenaProvinceData &getProvinceData(int index);
	const ArenaProvinceData &getProvinceData(int index) const;

	bool init(const char *filename);
};

#endif
