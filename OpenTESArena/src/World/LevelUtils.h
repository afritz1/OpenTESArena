#ifndef LEVEL_UTILS_H
#define LEVEL_UTILS_H

#include <cstdint>
#include <string>

#include "../Assets/CityDataFile.h"

class ArenaRandom;
class ExeData;

// Various functions for working with Arena's level data.

namespace LevelUtils
{
	// Gets the offset value of a door voxel in the world. Used with various calculations
	// (.MIF name, lock level).
	uint16_t getDoorVoxelOffset(int x, int y);

	// Gets the .MIF name for a door voxel in a city or the wilderness.
	std::string getDoorVoxelMifName(int x, int y, int menuID, int localCityID, int provinceID,
		const CityDataFile::ProvinceData &province, bool isCity, const ExeData &exeData);

	// Gets the lock level for a door voxel at the given XY coordinate.
	int getDoorVoxelLockLevel(int x, int y, ArenaRandom &random);

	// Gets the '#' number used in IN#.0x and RE#.0x save files.
	int getServiceSaveFileNumber(int doorX, int doorY);
	int getWildernessServiceSaveFileNumber(int wildX, int wildY);
}

#endif
