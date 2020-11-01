#ifndef LEVEL_UTILS_H
#define LEVEL_UTILS_H

#include <cstdint>
#include <string>

#include "LocationDefinition.h"
#include "VoxelUtils.h"
#include "../Assets/INFFile.h"
#include "../Assets/MIFFile.h"

#include "components/utilities/BufferView2D.h"

class ArenaRandom;
class ExeData;

// Various functions for working with Arena's level data.

namespace LevelUtils
{
	// Display names for *MENU transition voxels.
	using MenuNamesList = std::vector<std::pair<NewInt2, std::string>>;

	// Converts an Arena ceiling height from "centimeters" to modern coordinates (1.0 per voxel).
	double convertArenaCeilingHeight(int ceilingHeight);

	// Gets the number of voxels a MAP2 voxel occupies vertically (at least 1).
	int getMap2VoxelHeight(uint16_t map2Voxel);

	// Gets the max height from a set of MAP2 voxels.
	int getMap2Height(const BufferView2D<const ArenaTypes::VoxelID> &map2);

	// Gets the voxel height of a .MIF level with optional ceiling data.
	int getMifLevelHeight(const MIFFile::Level &level, const INFFile::CeilingData *ceiling);

	// Gets the offset value of a door voxel in the world. Used with various calculations
	// (.MIF name, lock level).
	uint16_t getDoorVoxelOffset(int x, int y);

	// Gets the .MIF name for a door voxel in a city or the wilderness.
	std::string getDoorVoxelMifName(int x, int y, int menuID, uint32_t rulerSeed,
		bool palaceIsMainQuestDungeon, LocationDefinition::CityDefinition::Type locationType,
		bool isCity, const ExeData &exeData);

	// Gets the lock level for a door voxel at the given XY coordinate.
	int getDoorVoxelLockLevel(int x, int y, ArenaRandom &random);

	// Gets the '#' number used in IN#.0x and RE#.0x save files.
	int getServiceSaveFileNumber(int doorX, int doorY);
	int getWildernessServiceSaveFileNumber(int wildX, int wildY);
}

#endif
